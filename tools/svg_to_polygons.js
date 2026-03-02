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

function quadraticPoint(p0, p1, p2, t) {
    const oneMinusT = 1 - t;
    const x = oneMinusT ** 2 * p0[0]
        + 2 * oneMinusT * t * p1[0]
        + t ** 2 * p2[0];
    const y = oneMinusT ** 2 * p0[1]
        + 2 * oneMinusT * t * p1[1]
        + t ** 2 * p2[1];
    return [x, y];
}

function vectorAngle(ux, uy, vx, vy) {
    const dot = ux * vx + uy * vy;
    const len = Math.sqrt((ux ** 2 + uy ** 2) * (vx ** 2 + vy ** 2));
    if (len === 0) {
        return 0;
    }

    const clamped = Math.max(-1, Math.min(1, dot / len));
    const sign = (ux * vy - uy * vx) >= 0 ? 1 : -1;
    return sign * Math.acos(clamped);
}

function sampleArcPoints(x1, y1, rxInput, ryInput, axisRotationDegrees, largeArcFlag, sweepFlag, x2, y2, samplesPerTurn = 96) {
    const rxAbs = Math.abs(rxInput);
    const ryAbs = Math.abs(ryInput);

    if (rxAbs <= 1e-12 || ryAbs <= 1e-12) {
        return [[x2, y2]];
    }

    if (Math.abs(x1 - x2) <= 1e-12 && Math.abs(y1 - y2) <= 1e-12) {
        return [];
    }

    const phi = axisRotationDegrees * (Math.PI / 180);
    const cosPhi = Math.cos(phi);
    const sinPhi = Math.sin(phi);

    const dx2 = (x1 - x2) / 2;
    const dy2 = (y1 - y2) / 2;

    const x1Prime = cosPhi * dx2 + sinPhi * dy2;
    const y1Prime = -sinPhi * dx2 + cosPhi * dy2;

    let rx = rxAbs;
    let ry = ryAbs;
    const lambda = (x1Prime ** 2) / (rx ** 2) + (y1Prime ** 2) / (ry ** 2);
    if (lambda > 1) {
        const scale = Math.sqrt(lambda);
        rx *= scale;
        ry *= scale;
    }

    const sign = (largeArcFlag === sweepFlag) ? -1 : 1;
    const numerator = (rx ** 2) * (ry ** 2) - (rx ** 2) * (y1Prime ** 2) - (ry ** 2) * (x1Prime ** 2);
    const denominator = (rx ** 2) * (y1Prime ** 2) + (ry ** 2) * (x1Prime ** 2);
    const ratio = denominator === 0 ? 0 : Math.max(0, numerator / denominator);
    const coefficient = sign * Math.sqrt(ratio);

    const cxPrime = coefficient * ((rx * y1Prime) / ry);
    const cyPrime = coefficient * (-(ry * x1Prime) / rx);

    const cx = cosPhi * cxPrime - sinPhi * cyPrime + (x1 + x2) / 2;
    const cy = sinPhi * cxPrime + cosPhi * cyPrime + (y1 + y2) / 2;

    const ux = (x1Prime - cxPrime) / rx;
    const uy = (y1Prime - cyPrime) / ry;
    const vx = (-x1Prime - cxPrime) / rx;
    const vy = (-y1Prime - cyPrime) / ry;

    const theta1 = vectorAngle(1, 0, ux, uy);
    let deltaTheta = vectorAngle(ux, uy, vx, vy);

    const shouldSweep = !!sweepFlag;
    if (!shouldSweep && deltaTheta > 0) {
        deltaTheta -= 2 * Math.PI;
    } else if (shouldSweep && deltaTheta < 0) {
        deltaTheta += 2 * Math.PI;
    }

    const segments = Math.max(1, Math.ceil((Math.abs(deltaTheta) / (2 * Math.PI)) * samplesPerTurn));
    const points = [];

    for (let i = 1; i <= segments; i += 1) {
        const t = theta1 + (deltaTheta * i) / segments;
        const cosT = Math.cos(t);
        const sinT = Math.sin(t);

        const x = cx + rx * cosPhi * cosT - ry * sinPhi * sinT;
        const y = cy + rx * sinPhi * cosT + ry * cosPhi * sinT;
        points.push([x, y]);
    }

    return points;
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
    let previousCubicControlX = 0;
    let previousCubicControlY = 0;
    let previousQuadraticControlX = 0;
    let previousQuadraticControlY = 0;

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
            previousCubicControlX = currentX;
            previousCubicControlY = currentY;
            previousQuadraticControlX = currentX;
            previousQuadraticControlY = currentY;
            continue;
        }

        if (code === 'L' || code === 'H' || code === 'V') {
            currentX = command.x;
            currentY = command.y;
            current.push(roundPoint([currentX, currentY]));
            previousCommand = 'L';
            previousCubicControlX = currentX;
            previousCubicControlY = currentY;
            previousQuadraticControlX = currentX;
            previousQuadraticControlY = currentY;
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
            previousCubicControlX = command.x2;
            previousCubicControlY = command.y2;
            previousQuadraticControlX = currentX;
            previousQuadraticControlY = currentY;
            previousCommand = 'C';
            continue;
        }

        if (code === 'S') {
            const reflectedControlX = (previousCommand === 'C' || previousCommand === 'S')
                ? (2 * currentX - previousCubicControlX)
                : currentX;
            const reflectedControlY = (previousCommand === 'C' || previousCommand === 'S')
                ? (2 * currentY - previousCubicControlY)
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
            previousCubicControlX = command.x2;
            previousCubicControlY = command.y2;
            previousQuadraticControlX = currentX;
            previousQuadraticControlY = currentY;
            previousCommand = 'S';
            continue;
        }

        if (code === 'Q') {
            const p0 = [currentX, currentY];
            const p1 = [command.x1, command.y1];
            const p2 = [command.x, command.y];

            for (let sampleIndex = 1; sampleIndex <= curveSamples; sampleIndex += 1) {
                const t = sampleIndex / curveSamples;
                current.push(roundPoint(quadraticPoint(p0, p1, p2, t)));
            }

            currentX = command.x;
            currentY = command.y;
            previousQuadraticControlX = command.x1;
            previousQuadraticControlY = command.y1;
            previousCubicControlX = currentX;
            previousCubicControlY = currentY;
            previousCommand = 'Q';
            continue;
        }

        if (code === 'T') {
            const reflectedControlX = (previousCommand === 'Q' || previousCommand === 'T')
                ? (2 * currentX - previousQuadraticControlX)
                : currentX;
            const reflectedControlY = (previousCommand === 'Q' || previousCommand === 'T')
                ? (2 * currentY - previousQuadraticControlY)
                : currentY;

            const p0 = [currentX, currentY];
            const p1 = [reflectedControlX, reflectedControlY];
            const p2 = [command.x, command.y];

            for (let sampleIndex = 1; sampleIndex <= curveSamples; sampleIndex += 1) {
                const t = sampleIndex / curveSamples;
                current.push(roundPoint(quadraticPoint(p0, p1, p2, t)));
            }

            currentX = command.x;
            currentY = command.y;
            previousQuadraticControlX = reflectedControlX;
            previousQuadraticControlY = reflectedControlY;
            previousCubicControlX = currentX;
            previousCubicControlY = currentY;
            previousCommand = 'T';
            continue;
        }

        if (code === 'A') {
            const arcPoints = sampleArcPoints(
                currentX,
                currentY,
                command.rx,
                command.ry,
                command.xAxisRotation || 0,
                command.largeArc || 0,
                command.sweep || 0,
                command.x,
                command.y,
                120,
            );

            for (const point of arcPoints) {
                current.push(roundPoint(point));
            }

            currentX = command.x;
            currentY = command.y;
            previousCubicControlX = currentX;
            previousCubicControlY = currentY;
            previousQuadraticControlX = currentX;
            previousQuadraticControlY = currentY;
            previousCommand = 'A';
            continue;
        }

        if (code === 'Z') {
            closed = true;
            if (current.length > 0) {
                current.push(roundPoint([startX, startY]));
            }
            previousCommand = 'Z';
            previousCubicControlX = currentX;
            previousCubicControlY = currentY;
            previousQuadraticControlX = currentX;
            previousQuadraticControlY = currentY;
            continue;
        }
    }

    pushCurrentIfValid();
    return polygons;
}

function convertPathToPolygonsWithFallback(pathData) {
    let hasArcCommands = false;
    try {
        hasArcCommands = parseSVG(pathData).some((command) => String(command.code || '').toUpperCase() === 'A');
    } catch (error) {
        hasArcCommands = /(^|[\s,])(?:[Aa])(?=[\s,\-+\d\.])/.test(pathData);
    }

    if (hasArcCommands) {
        console.warn('Path contains arc commands (A/a); using manual converter.');
        return manualPathToPolygons(pathData);
    }

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
            const hasUnsupportedArc = message.toLowerCase().includes('elliptical arc commands')
                || message.includes('commands (A) are not yet supported')
                || message.includes('commands (a) are not yet supported');

            if (hasUnsupportedArc) {
                console.warn('Conversion library does not support arc commands; using manual converter.');
                break;
            }

            if (!isStackOverflow) {
                throw error;
            }
            console.warn(`Conversion failed at tolerance=${tolerance}, retrying with coarser sampling...`);
        }
    }

    console.warn('Falling back to manual non-recursive path sampling...');
    return manualPathToPolygons(pathData);
}

function ellipseElementToPolygon(element, samples = 120) {
    const properties = (element && element.properties) || {};

    const cx = Number(properties.cx || 0);
    const cy = Number(properties.cy || 0);
    const rxRaw = properties.rx !== undefined ? properties.rx : properties.r;
    const ryRaw = properties.ry !== undefined ? properties.ry : properties.r;
    const rx = Number(rxRaw);
    const ry = Number(ryRaw);

    if (!Number.isFinite(cx) || !Number.isFinite(cy) || !Number.isFinite(rx) || !Number.isFinite(ry)) {
        return [];
    }

    if (rx <= 0 || ry <= 0) {
        return [];
    }

    const points = [];
    for (let i = 0; i < samples; i += 1) {
        const angle = (2 * Math.PI * i) / samples;
        points.push([cx + rx * Math.cos(angle), cy + ry * Math.sin(angle)]);
    }

    points.closed = true;
    return [points];
}

// Parse command line arguments
const args = process.argv.slice(2);
let inputFile = args[0] || "test.svg";
let outputFile = args[1] || null;

// Resolve input file path
const inputPath = path.resolve(inputFile);

if (!fs.existsSync(inputPath)) {
    console.error(`Error: Input file not found: ${inputPath}`);
    process.exit(1);
}

console.log(`Reading SVG from: ${inputPath}`);
let svg = fs.readFileSync(inputPath, 'utf8')

const parsed_svg = parse(svg);

function findConvertibleElementsFromSVGString(svg_string) {
    const elements = [];

    function traverse(node) {
        if (node && node.type === "element") {
            if (node.tagName === "path" && node.properties && typeof node.properties.d === 'string') {
                elements.push({ type: 'path', node });
            } else if (node.tagName === "ellipse" || node.tagName === "circle") {
                elements.push({ type: 'ellipse', node });
            }
        }

        if (node && node.children && Array.isArray(node.children)) {
            for (const child of node.children) {
                traverse(child);
            }
        }
    }

    traverse(svg_string);
    return elements;
}

const convertibleElements = findConvertibleElementsFromSVGString(parsed_svg);

// Collect all polygons from all paths into a single array
const allPolygons = [];

convertibleElements.forEach((entry, index) => {
    let points = [];

    if (entry.type === 'path') {
        const pathData = entry.node.properties.d;
        points = convertPathToPolygonsWithFallback(pathData);
        console.log(`Element ${index + 1}: path generated ${points.length} polygon(s)`);
    } else if (entry.type === 'ellipse') {
        points = ellipseElementToPolygon(entry.node);
        console.log(`Element ${index + 1}: ${entry.node.tagName} generated ${points.length} polygon(s)`);
    }

    const polygons = points
        .map((poly) => {
            const normalizedPoints = cleanAndNormalizePolygon(poly, true);
            return {
                points: normalizedPoints,
                closed: poly.closed === true,
                winding: 'CW',
            };
        })
        .filter((poly) => poly.points.length >= 3);

    console.log(`Element ${index + 1}: kept ${polygons.length} normalized polygon(s)`);
    allPolygons.push(...polygons);
});

// Apply layering order to the merged set of polygons
const jsonOutput = orderPolygonsForLayering(allPolygons);

// Generate output filename
const outFile = outputFile
    ? outputFile
    : 'output.json';

fs.writeFileSync(outFile, JSON.stringify(jsonOutput, null, 2), 'utf8');
console.log(`\nConversion complete! Produced ${jsonOutput.length} polygon(s).`);
console.log(`Processed ${convertibleElements.length} convertible element(s) (path/ellipse/circle).`);
console.log(`Wrote: ${outFile}`);