const { pathDataToPolys } = require('svg-path-to-polygons');
const { parse } = require('svg-parser');
const fs = require('fs');
const path = require('path');

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
    const jsonOutput = points.map((poly) => ({
        points: poly,
        closed: poly.closed === true,
    }));

    index += 1;

    // Generate output filename
    const outFile = outputPrefix 
        ? `${outputPrefix}${index}.json`
        : `test${index}.json`;

    fs.writeFileSync(outFile, JSON.stringify(jsonOutput, null, 2), 'utf8');
    console.log(`Wrote: ${outFile}`);
});

console.log(`\nConversion complete! Generated ${index} JSON file(s).`);
