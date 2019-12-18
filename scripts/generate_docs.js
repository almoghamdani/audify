const TypeDoc = require('typedoc');
const git = require('simple-git')();

const outputDir = 'docs';
const app = new TypeDoc.Application({
	mode: 'file',
	ignoreCompilerErrors: true,
	includeDeclarations: true,
	exclude: "**/node_modules/**",
	theme: "node_modules/eledoc/bin/default/",
	hideGenerator: true,
	name: "Audify.js"
});

const project = app.convert(app.expandInputFiles(['.']));

// Project may not have converted correctly
if (project) {
	// Generate JSON output for comparsion
	app.generateJson(project, outputDir + '/documentation.json');

	// Check if the JSON changed
	git.diffSummary((_, summary) => {
		const docsChanged = summary.files.filter(file => file.file === outputDir + '/documentation.json').length !== 0;

		// If the docs changed, render them and commit them
		if (docsChanged) {
			// Render docs
			app.generateDocs(project, outputDir);

			// Commit the new docs
			git.add("docs").commit("chore: regenerate docs");
		} else {
			// Checkout the JSON file
			git.checkout(outputDir + '/documentation.json');
		}
	})
}