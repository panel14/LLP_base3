#include "interactiveMode.h"

FILE* initialize(int argc, char** argv) {
    char* filename;
    char* v;

    FILE* fp;
    FILE* parsed;

    char flag;

    if (argc < 3 || argc > 4) {
        printf("Wrong number of aguments: %d", argc);
        return NULL;
    }
    else if (argc == 4) {
        v = argv[3];
    }
    filename = argv[2];
    flag = argv[1][1];

    switch (flag)
    {
        case 'o':
            storageOpenOrCreateFile(filename, "r+b", &fp);
            break;

        case 'n':
            storageOpenOrCreateFile(filename, "w", &fp);
            storageCloseFile(fp);
            storageOpenOrCreateFile(filename, "r+b", &fp);
            initFile(fp);
            break;

        case 'p':
            storageOpenOrCreateFile(v, "r", &parsed);

            storageOpenOrCreateFile(filename, "w", &fp);
            storageCloseFile(fp);
            storageOpenOrCreateFile(filename, "r+b", &fp);

            initFile(fp);
            parseFile(fp, parsed);
            break;
        default:
            printf("unknown");
            break;
    }
    return fp;
}

void initFile(FILE* fp) {

    printf("Initializing template.\nInput the number of attributes in nodes: ");
    char* count_str = malloc(INPUT_SIZE);
    scanf("%s", count_str);
    while (!isNumeric(count_str)) {
        printf("Incorrect input, try again: ");
        scanf("%s", count_str);
    }
    size_t count = strtol(count_str, NULL, 10);
    char* str;
    char** str_array = malloc(count * sizeof(char*));
    char* type = malloc(INPUT_SIZE);
    uint32_t* types = malloc(count * sizeof(uint32_t));
    size_t* sizes = malloc(count * sizeof(size_t));
    size_t temp_size;
    for (size_t iter = 0; iter < count; iter++) {
        printf("<---- Attribute %-3zu ---->\n", iter);
        str = malloc(INPUT_SIZE);
        printf("Attribute name: ");
        scanf("%s", str);
        str_array[iter] = str;
        temp_size = strlen(str);
        sizes[iter] = temp_size + (!(temp_size % FILE_GRANULARITY) ? 1 : 0);
        printf("%d. Integer type\n", INT);
        printf("%d. Float type\n", FLOAT);
        printf("%d. String type\n", STRING);
        printf("%d. Boolean type\n", BOOL);
        printf("Choose field type: ");
        scanf("%s", type);
        while (strlen(type) != 1) {
            printf("Incorrect input, try again: ");
            scanf("%s", type);
        }
        types[iter] = strtol(type, NULL, 10);
    }


    initEmptyFile(fp, str_array, types, count, sizes);

    for (size_t iter = 0; iter < count; iter++) free(str_array[iter]);
    free(str_array);
    free(sizes);
    free(types);
    free(count_str);
    free(type);
}

void start(FILE* fp) {
    size_t templateSize;
    
    struct treeSchema* schema = malloc(SCHEMA_SIZE);
    readTreeSchema(schema, fp);

    templateSize = schema->meta->templateSize;
    uint32_t* types= malloc(sizeof(uint32_t) * templateSize);
    char** attrNames = malloc(sizeof(char*) * templateSize);

    for (int i = 0; i < templateSize; i++) {
        types[i] = schema->nodesTemplate[i]->header->type;
        attrNames[i] = schema->nodesTemplate[i]->attributeName;
    }


    printf("File opened successfully!\n");
    printf("Type 'help' for available commands info.\n");

    char* line = malloc(INPUT_SIZE);
    size_t length = 0;
    size_t count;
    char** argv;

    getline(&line, &length, stdin);

    if (strcmp(line, "\n") == 0) {
        getline(&line, &length, stdin);
    }
    count = split(line, ' ', &argv);

    while (strcmp(argv[0], "exit") != 0) {

        if (strcmp(argv[0], "help") == 0) {
            storageGetHelp();
        }

        else if (strcmp(argv[0], "find_by") == 0) {
            storageRetriveNode(fp, argv, templateSize, types, attrNames, count);
        }
        else if (strcmp(argv[0], "add") == 0) {

            if (count == templateSize + 2) {
                size_t code = storageInsertNode(fp, argv, templateSize, types, attrNames);
                if (code != 0) {
                    printf("Error code: %zu\n", code);
                }
                else printf("Node added\n");
            }
            else
                printf("Wrong number of parameters(including parent_id): %lu expected, %lu entered.\n",
                    templateSize + 1, count - 1);

        }
        else if (strcmp(argv[0], "update") == 0) {
            
            size_t code = storageUpdateNode(fp, argv, templateSize, types, attrNames, count);
            if (code != 0) {
                printf("Error code: %zu\n", code);
            }
            else printf("Node updated\n");
        }
        else if (strcmp(argv[0], "remove") == 0) {
            if (count == 2) {
                if (storageDeleteNode(fp, argv[1], 0) == 0) printf("Node removed\n");
            }
            else if (count == 1)
                printf("Missing parameter <node_id>.\n");
            else printf("Too many parameters.\n");
        }
        else if (strcmp(argv[0], "print") == 0) {
            if (count == 2) {
                if (strcmp(argv[1], "schema") == 0) printTreeSchema(fp);
                else if (strcmp(argv[1], "nodes") == 0) printNodesFrom(fp);
                else printf("Incorrect parameter ([print header]/[print array]).\n");
            }
            else if (count == 1) printf("Missing parameter ([print header]/[print array]).\n");
            else printf("Too many parameters ([print header]/[print array]).\n");

        }
        else printf("Unknown command, try using 'help'\n");

        free(argv);
        getline(&line, &length, stdin);
        count = split(line, ' ', &argv);
    }
    storageCloseFile(fp);
}