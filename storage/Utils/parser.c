#include "parser.h"

void parseFile(FILE* to, FILE* from) {
	char line[INPUT_SIZE];
	char** args = NULL;
	size_t templateSize;

	struct treeSchema* schema = malloc(SCHEMA_SIZE);
	readTreeSchema(schema, to);

	templateSize = schema->meta->templateSize;
	uint32_t* types = malloc(sizeof(uint32_t) * templateSize);
	char** attrNames = malloc(sizeof(char *) * templateSize);

	for (int i = 0; i < templateSize; i++) {
		types[i] = schema->nodesTemplate[i]->header->type;
		attrNames[i] = schema->nodesTemplate[i]->attributeName;
	}

	fgets(line, INPUT_SIZE, from);

	size_t iCount = 1;

	while (!feof(from)) {
		if (strlen(line) == 0) break;
		line[strlen(line) - 1] = '\0';
		char* prefix = concat("add ", line);
		split(prefix, ' ', &args);


		//clock_t begin = clock();
		storageInsertNode(to, args, templateSize, types, attrNames);
		//clock_t end = clock();

		//printf("%zu %f\n", iCount++, (double)(end - begin) / CLOCKS_PER_SEC);

		free(prefix);
		free(args);
		fgets(line, INPUT_SIZE, from);
	}

	freeTreeSchema(schema);
	fclose(from);
	fflush(to);
	free(types);
	free(attrNames);
}