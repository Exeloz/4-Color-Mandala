const { pathDataToPolys } = require('svg-path-to-polygons');
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

    let points = pathDataToPolys(newPathData, {tolerance:1, decimals:3});
    const jsonOutput = points
        .map((poly) => {
            const normalizedPoints = cleanAndNormalizePolygon(poly, true);
            return {
                points: normalizedPoints,
                closed: poly.closed === true,
                winding: 'CW',
            };
        })
        .filter((poly) => poly.points.length >= 3);

    index += 1;

    // Generate output filename
    const outFile = outputPrefix 
        ? `${outputPrefix}${index}.json`
        : `test${index}.json`;

    fs.writeFileSync(outFile, JSON.stringify(jsonOutput, null, 2), 'utf8');
    console.log(`Wrote: ${outFile}`);
});

console.log(`\nConversion complete! Generated ${index} JSON file(s).`);
