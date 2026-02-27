const { pathDataToPolys } = require('svg-path-to-polygons');
const { parseSVG, makeAbsolute } = require('svg-path-parser');
const { parse } = require('svg-parser');
const fs = require('fs');
const path = require('path');

function pointsEqual(a, b, epsilon = 1e-6) {
    return Math.abs(a[0] - b[0]) <= epsilon && Math.abs(a[1] - b[1]) <= epsilon;
}

function signedArea(points) {
    if (!Array.isArray(points) || points.length < 3) {
        return 0;
    }

    let areaTimesTwo = 0;
    for (let i = 0; i < points.length; i += 1) {
        const current = points[i];
        const next = points[(i + 1) % points.length];
        areaTimesTwo += current[0] * next[1] - next[0] * current[1];
    }
    return areaTimesTwo * 0.5;
}

function cleanAndNormalizePolygon(points, targetClockwise = false) {
    if (!Array.isArray(points)) {
        return [];
    }

    const cleaned = [];
    for (const point of points) {
        if (!Array.isArray(point) || point.length < 2) {
            continue;
        }

        const candidate = [Number(point[0]), Number(point[1])];
        if (!Number.isFinite(candidate[0]) || !Number.isFinite(candidate[1])) {
            continue;
        }

        if (cleaned.length === 0 || !pointsEqual(cleaned[cleaned.length - 1], candidate)) {
            cleaned.push(candidate);
        }
    }

    if (cleaned.length >= 2 && pointsEqual(cleaned[0], cleaned[cleaned.length - 1])) {
        cleaned.pop();
    }

    if (cleaned.length < 3) {
        return [];
    }

    const area = signedArea(cleaned);
    const isClockwise = area < 0;
    if (isClockwise !== targetClockwise) {
        cleaned.reverse();
    }

    return cleaned;
}

function boundsOf(points) {
    let minX = points[0][0];
    let minY = points[0][1];
    let maxX = points[0][0];
    let maxY = points[0][1];
    for (const [x, y] of points) {
        if (x < minX) minX = x;
        if (y < minY) minY = y;
        if (x > maxX) maxX = x;
        if (y > maxY) maxY = y;
    }
    return { minX, minY, maxX, maxY };
}

function boundsContained(inner, outer, epsilon = 1e-6) {
    return inner.minX >= outer.minX - epsilon
        && inner.minY >= outer.minY - epsilon
        && inner.maxX <= outer.maxX + epsilon
        && inner.maxY <= outer.maxY + epsilon;
}

function pointInPolygonOrOnEdge(point, polygon) {
    const [px, py] = point;
    let inside = false;

    for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i, i += 1) {
        const [x1, y1] = polygon[i];
        const [x2, y2] = polygon[j];

        const cross = (py - y1) * (x2 - x1) - (px - x1) * (y2 - y1);
        const onSegment = Math.abs(cross) <= 1e-6
            && px >= Math.min(x1, x2) - 1e-6
            && px <= Math.max(x1, x2) + 1e-6
            && py >= Math.min(y1, y2) - 1e-6
            && py <= Math.max(y1, y2) + 1e-6;
        if (onSegment) {
            return true;
        }

        const intersects = ((y1 > py) !== (y2 > py))
            && (px < (x2 - x1) * (py - y1) / ((y2 - y1) || 1e-12) + x1);
        if (intersects) {
            inside = !inside;
        }
    }

    return inside;
}

function polygonInsidePolygon(innerPoints, outerPoints, innerBounds, outerBounds) {
    if (!boundsContained(innerBounds, outerBounds)) {
        return false;
    }

    const sampleCount = Math.min(24, innerPoints.length);
    const step = Math.max(1, Math.floor(innerPoints.length / sampleCount));
    for (let i = 0; i < innerPoints.length; i += step) {
        if (!pointInPolygonOrOnEdge(innerPoints[i], outerPoints)) {
            return false;
        }
    }

    return true;
}

function orderPolygonsForLayering(polygons) {
    const count = polygons.length;
    if (count < 2) {
        return polygons;
    }

    const bounds = polygons.map((poly) => boundsOf(poly.points));
    const edges = Array.from({ length: count }, () => []);
    const indegree = Array.from({ length: count }, () => 0);

    for (let innerIndex = 0; innerIndex < count; innerIndex += 1) {
        for (let outerIndex = 0; outerIndex < count; outerIndex += 1) {
            if (innerIndex === outerIndex) {
                continue;
            }

            if (!polygonInsidePolygon(polygons[innerIndex].points, polygons[outerIndex].points, bounds[innerIndex], bounds[outerIndex])) {
                continue;
            }

            edges[outerIndex].push(innerIndex);
            indegree[innerIndex] += 1;
        }
    }

    const queue = [];
    for (let index = 0; index < count; index += 1) {
        if (indegree[index] === 0) {
            queue.push(index);
        }
    }
    queue.sort((a, b) => a - b);

    const orderedIndices = [];
    while (queue.length > 0) {
        const current = queue.shift();
        orderedIndices.push(current);
        for (const next of edges[current]) {
            indegree[next] -= 1;
            if (indegree[next] === 0) {
                queue.push(next);
            }
        }
        queue.sort((a, b) => a - b);
    }

    if (orderedIndices.length !== count) {
        return polygons;
    }

    return orderedIndices.map((index) => polygons[index]);
}

function cubicPoint(p0, p1, p2, p3, t) {
    const oneMinusT = 1 - t;
    const x = oneMinusT ** 3 * p0[0]
        + 3 * oneMinusT ** 2 * t * p1[0]
        + 3 * oneMinusT * t ** 2 * p2[0]
        + t ** 3 * p3[0];
    const y = oneMinusT ** 3 * p0[1]
        + 3 * oneMinusT ** 2 * t * p1[1]
        + 3 * oneMinusT * t ** 2 * p2[1]
        + t ** 3 * p3[1];
    return [x, y];
}

function manualPathToPolygons(pathData, decimals = 3, curveSamples = 20) {
    const commands = makeAbsolute(parseSVG(pathData));
    const polygons = [];

    let current = [];
    let closed = false;
    let currentX = 0;
    let currentY = 0;
    let startX = 0;
    let startY = 0;
    let previousCommand = '';
    let previousControlX = 0;
    let previousControlY = 0;

    const roundPoint = (pt) => [
        Number(pt[0].toFixed(decimals)),
        Number(pt[1].toFixed(decimals)),
    ];

    const pushCurrentIfValid = () => {
        if (current.length >= 3) {
            const poly = current.slice();
            poly.closed = closed;
            polygons.push(poly);
        }
        current = [];
        closed = false;
    };

    for (const command of commands) {
        const code = String(command.code || '').toUpperCase();

        if (code === 'M') {
            pushCurrentIfValid();
            currentX = command.x;
            currentY = command.y;
            startX = currentX;
            startY = currentY;
            current.push(roundPoint([currentX, currentY]));
            previousCommand = 'M';
            continue;
        }

        if (code === 'L' || code === 'H' || code === 'V') {
            currentX = command.x;
            currentY = command.y;
            current.push(roundPoint([currentX, currentY]));
            previousCommand = 'L';
            continue;
        }

        if (code === 'C') {
            const p0 = [currentX, currentY];
            const p1 = [command.x1, command.y1];
            const p2 = [command.x2, command.y2];
            const p3 = [command.x, command.y];

            for (let sampleIndex = 1; sampleIndex <= curveSamples; sampleIndex += 1) {
                const t = sampleIndex / curveSamples;
                current.push(roundPoint(cubicPoint(p0, p1, p2, p3, t)));
            }

            currentX = command.x;
            currentY = command.y;
            previousControlX = command.x2;
            previousControlY = command.y2;
            previousCommand = 'C';
            continue;
        }

        if (code === 'S') {
            const reflectedControlX = (previousCommand === 'C' || previousCommand === 'S')
                ? (2 * currentX - previousControlX)
                : currentX;
            const reflectedControlY = (previousCommand === 'C' || previousCommand === 'S')
                ? (2 * currentY - previousControlY)
                : currentY;

            const p0 = [currentX, currentY];
            const p1 = [reflectedControlX, reflectedControlY];
            const p2 = [command.x2, command.y2];
            const p3 = [command.x, command.y];

            for (let sampleIndex = 1; sampleIndex <= curveSamples; sampleIndex += 1) {
                const t = sampleIndex / curveSamples;
                current.push(roundPoint(cubicPoint(p0, p1, p2, p3, t)));
            }

            currentX = command.x;
            currentY = command.y;
            previousControlX = command.x2;
            previousControlY = command.y2;
            previousCommand = 'S';
            continue;
        }

        if (code === 'Z') {
            closed = true;
            if (current.length > 0) {
                current.push(roundPoint([startX, startY]));
            }
            previousCommand = 'Z';
            continue;
        }
    }

    pushCurrentIfValid();
    return polygons;
}

function convertPathToPolygonsWithFallback(pathData) {
    const tolerances = [1, 2, 4, 8, 12, 16];
    let lastError = null;

    for (const tolerance of tolerances) {
        try {
            const polygons = pathDataToPolys(pathData, { tolerance, decimals: 3 });
            if (tolerance > 1) {
                console.log(`Conversion succeeded with fallback tolerance=${tolerance}`);
            }
            return polygons;
        } catch (error) {
            lastError = error;
            const message = String(error && error.message ? error.message : error);
            const isStackOverflow = message.includes('Maximum call stack size exceeded');
            if (!isStackOverflow) {
                throw error;
            }
            console.warn(`Conversion failed at tolerance=${tolerance}, retrying with coarser sampling...`);
        }
    }

    console.warn('Falling back to manual non-recursive path sampling...');
    return manualPathToPolygons(pathData);
}

// Parse command line arguments
const args = process.argv.slice(2);
let inputFile = args[0] || "test.svg";
let outputPrefix = args[1] || null;

// Resolve input file path
const inputPath = path.resolve(inputFile);

if (!fs.existsSync(inputPath)) {
    console.error(`Error: Input file not found: ${inputPath}`);
    process.exit(1);
}

console.log(`Reading SVG from: ${inputPath}`);
let svg = fs.readFileSync(inputPath, 'utf8')

const parsed_svg = parse(svg);

function findPathFromSVGString(svg_string) {
    const paths = [];

    function traverse(node) {
        if (node && node.type === "element" && node.tagName === "path") {
            paths.push(node);
        }

        if (node && node.children && Array.isArray(node.children)) {
            for (const child of node.children) {
                traverse(child);
            }
        }
    }

    traverse(svg_string);
    return paths;
}

let pathsData = findPathFromSVGString(parsed_svg);

function convertQuadraticToCubic(pathData) {
    const quadraticPattern = /q([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)[,\s]*([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)[,\s]*([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)[,\s]*([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)/gi;
	
    const matches = [...pathData.matchAll(quadraticPattern)];
	
	const conversions = matches.map(match => {
		const x1 = parseFloat(match[1]); 		const y1 = parseFloat(match[2]); 		const x = parseFloat(match[3]);  		const y = parseFloat(match[4]);  		
														const cx1 = (2/3) * x1;
		const cy1 = (2/3) * y1;
		const cx2 = x + (1/3) * x1;
		const cy2 = y + (1/3) * y1;
		
		return {
			original: match[0],
			cubic: `c${cx1.toFixed(2)},${cy1.toFixed(2)} ${cx2.toFixed(2)},${cy2.toFixed(2)} ${x},${y}`,
			params: { x1, y1, x, y, cx1, cy1, cx2, cy2 }
		};
	});
	
	return conversions;
}

function replaceQuadraticWithCubic(pathData) {
		const quadraticPattern = /q([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)[,\s]*([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)[,\s]*([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)[,\s]*([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)/gi;
	
	const replacedPath = pathData.replace(quadraticPattern, (match, x1, y1, x, y) => {
		const x1_num = parseFloat(x1);
		const y1_num = parseFloat(y1);
		const x_num = parseFloat(x);
		const y_num = parseFloat(y);
		
				const cx1 = (2/3) * x1_num;
		const cy1 = (2/3) * y1_num;
		const cx2 = x_num + (1/3) * x1_num;
		const cy2 = y_num + (1/3) * y1_num;
		
		return `c${cx1.toFixed(2)},${cy1.toFixed(2)} ${cx2.toFixed(2)},${cy2.toFixed(2)} ${x_num},${y_num}`;
	});
	
	return replacedPath;
}

let index = 0 ;

pathsData.forEach(pathData => {
    pathData = pathData["properties"]['d']

    const conversions = convertQuadraticToCubic(pathData);
    console.log('Found', conversions.length, 'quadratic curves to convert');
    if (conversions.length > 0) {
        console.log('First conversion:', conversions[0]);
    }

    const newPathData = replaceQuadraticWithCubic(pathData);
    console.log('Path conversion complete. Original length:', pathData.length, 'New length:', newPathData.length);

    let points = convertPathToPolygonsWithFallback(newPathData);
    const jsonOutput = orderPolygonsForLayering(points
        .map((poly) => {
            const normalizedPoints = cleanAndNormalizePolygon(poly, true);
            return {
                points: normalizedPoints,
                closed: poly.closed === true,
                winding: 'CW',
            };
        })
        .filter((poly) => poly.points.length >= 3));

    index += 1;

    // Generate output filename
    const outFile = outputPrefix 
        ? `${outputPrefix}${index}.json`
        : `test${index}.json`;

    fs.writeFileSync(outFile, JSON.stringify(jsonOutput, null, 2), 'utf8');
    console.log(`Wrote: ${outFile}`);
});

console.log(`\nConversion complete! Generated ${index} JSON file(s).`);
