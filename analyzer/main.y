%{
#include "include/main.h"

struct query query = {0};
struct query emptyQuery = {0};
struct fullComparator* cmp;

size_t vtype;
size_t bytesCount = 0;

int socket, numBytes;

#define MAX_DATA_SIZE 100
#define PORT 3939
%}

%union {uint64_t vint; char *string; float vfloat;}
%token STORAGE
%token RECEIVE INSERT DELETE UPDATE
%token <string> PARENT STRING
%token SET OR
%token LO_T LO_EQ_T GR_T GR_EQ_T NO_EQ
%token OP_BRACE CL_BRACE
%token OP_C_BRACE CL_C_BRACE
%token OP_SQ_BRACE CL_SQ_BRACE
%token COLON DOLLAR COMMA QUOTE
%token <vint> FALSE TRUE INT
%token <vfloat> FLOAT
%type <vint> bool value operation comp

%%

syntax: printer syntax;
    |
    ;

printer: mongosh {sendData();};

mongosh: STORAGE RECEIVE OP_BRACE OP_C_BRACE filters CL_C_BRACE CL_BRACE {setCommand(0);}
|
STORAGE DELETE OP_BRACE OP_C_BRACE filters CL_C_BRACE CL_BRACE {setCommand(1);}
|
STORAGE INSERT OP_BRACE parent_def COMMA vals_def CL_C_BRACE {setCommand(2);}
|
STORAGE UPDATE OP_BRACE OP_C_BRACE filters CL_C_BRACE COMMA DOLLAR SET COLON vals_def CL_BRACE {setCommand(3);};

parent_def : OP_C_BRACE PARENT COLON INT CL_C_BRACE {setCurOper(0);
    vtype = INT_TYPE;
    setCurVal("parent", $4, 0);
    switchFilter();};

vals_def : OP_C_BRACE set_vals CL_C_BRACE;

filters : filter {switchFilter();} | filter COMMA filters {switchFilter();};

filter : STRING COLON value {
    setCurOper(0);
    float val;
    if (vtype == FLOAT_TYPE){
        memcpy(&val, &$3, sizeof(uint64_t));
        setCurVal($1, 0, val);
    }else {
        setCurVal($1, $3, 0);
    }
}
|
STRING COLON operation {setCurVal($1, $3, 0);}
|
DOLLAR OR OP_SQ_BRACE filter COMMA filter CL_SQ_BRACE {setComp();}
;

operation: OP_C_BRACE DOLLAR comp COLON value CL_C_BRACE {setCurOper($3); $$ = $5;};

set_vals : set_val | set_val COMMA set_vals

set_val : STRING COLON value {
    if (vtype == FLOAT_TYPE){
        float val;
        memcpy(&val, &$3, sizeof(uint64_t));
        addValSetting($1, 0, val);
    }else {
        addValSetting($1, $3, 0);
    }
};

value : QUOTE STRING QUOTE {vtype = STRING_TYPE; $$ = $2;}
|
INT {vtype = INT_TYPE; $$ = $1;}
|
FLOAT {vtype = FLOAT_TYPE; memcpy(&$$, &$1, sizeof(uint64_t));}
|
bool {vtype = INT_TYPE; $$ = $1;};

bool : TRUE {$$ = 1;} | FALSE {$$ = 0;};

comp : LO_T {$$ = 1;}
|
LO_EQ_T {$$ = 2;}
|
GR_T {$$ = 3;}
|
GR_EQ_T {$$ = 4;}
|
NO_EQ {$$ = 5;};
%%

int main (int c, char** v) {

    if (c != 2) {
        printf("Only 1 argument expected: host address\n");
    }

    struct sockaddr_in local;
    char* path = NULL;

    if (c > 1) path = v[1];

    socket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = inet_addr(v[1]);
    local.sin_port = htons(PORT);

    if (connect(socket, (struct sockaddr*) &local, sizeof(local)) != 0) {
        perror("Connection error!");
        return 1;
    }

    printf("Client: connecting...\n");

    while (1) {
        yyparse();
    }

    close(socket);

    return 0;
}

void parseQuery(Query* qtree) {
    qtree->command = query.command;

    size_t comparatorIndex = 0;
    size_t filterIndex = 0;

    while(query.filtersList) {
        if (!query.filtersList->compList) {
            query.filtersList = query.filters->next;
            continue;
        }
        Query_Filter filter = Query_Filter_init_zero;
        while(query.filtersList->compList) {
            Query_Comparator comp = Query_Comparator_init_zero;

            Query_KeyValuePair kv = Query_KeyValuePair_init_zero;
            kv.valueType = query.filtersList->compList->kv.valueType;
            kv.valueInt = query.filtersList->compList->kv.valueInt;
            kv.valueReal = query.filtersList->compList->kv.valueReal;

            if (kv.valueType == STRING_TYPE)
                strcpy(kv.valueString, query.filtersList->compList->kv.valueInt);
            strcpy(kv.key, query.filtersList->compList->kv.key);
            comp.kv = kv;
            comp.operation = query.filtersList->compList->operation;
            filter.compList[comparatorIndex++] = comp;
            query.filtersList->compList = query.filtersList->compList->next;
        }
        filter.compList_count = comparatorIndex;
        qtree->filtersList[filterIndex++] = filter;

        comparatorIndex = 0;
        query.filtersList = query.filtersList->next;
    }

    qtree->filtersList_count = filterIndex;

    size_t settingsIndex = 0;

    while(query.settingsList) {
        Query_ValueSetting val = Query_ValueSetting_init_zero;

        Query_KeyValuePair kv = Query_KeyValuePair_init_zero;
        kv.valueType = query.settingsList->kv.valueType;
        kv.valueInt = query.settingsList->kv.valueInt;
        kv.valueReal = query.settingsList->kv.valueReal;

        if (kv.valueType == STRING_TYPE) {
            strcpy(kv.valueString, query.settingsList->kv.valueInt);
        }
        strcpy(kv.key, query.settingsList->kv.key);
        val.kv = kv;

        qtree->settings[settingsIndex++] = val;
        query.settingsList = query.settingsList->next;
    }
    qtree->settingsList_count = settingsIndex;
}

void sendData() {
    Query qtree = Query_init_zero;

    parseQuery(&qtree);

    pb_ostream_t out = pbOstreamFromSocket(socket);
    if (!pb_encode_delimited(&out, Query_fields, &qtree)) {
        fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&out));
    }

    Response res = {};

    pb_istream_t in = pbIstreamFromSocket(socket);
    if (!pb_decode_delimited(&in, Response_fields, &res)) {
        printf("Decode failed: %s\n", PB_GET_ERROR(&in));
        return 2;
    }

    printf("%s", res.rString);

    while(!r.last) {
        if (!pb_decode_delimited(&in, Response_fields, &res)) {
            printf("Decode failed: %s\n", PB_GET_ERROR(&in));
            return 2;
        }
        printf("%s", res.rString);
    }

    query = emptyQuery;
}

void *dmalloc(size_t sizeOf){
    bytesCount += sizeOf;
    return malloc(sizeOf);
}

void printMemory(){
    printf("Memory usage: %zu bytes; %zu filters;\n", bytesCount, filtersCount);
}

void addValSetting(char* key, uint64_t valInt, double valReal){
    struct valueSetting* vs = dmalloc(VS_SIZE);
    struct keyValuePair kv = {.key = key, .valueType = vtype};
    kv.valueReal = valReal;
    kv.valueInt = valInt;
    vs->kv = kv;
    vs->next = query.settingsList;
    query.settingsList = vs;

}

void setCurOper(uint8_t operation){
    struct fullComparator* tmp = dmalloc(FULL_COMP_SIZE);
    tmp->next = cmp;
    tmp->operation = operation;
    cmp = tmp;

}

void setCurVal(char* key, uint64_t valInt, double valReal){
    struct keyValuePair kv = {.key = key, .valueType = vtype};
    kv.valueReal = valReal;
    kv.valueInt = valInt;
    cmp->kv = kv;
}

void switchFilter(){
    struct filter* f = dmalloc(FILTER_SIZE);
    struct comparator* tmp = dmalloc(COMP_SIZE);
    f->next = query.filtersList;

    if (cmp->connected){
        tmp->next = dmalloc(COMP_SIZE);
        tmp->next->operation = cmp->connected->operation;
        tmp->next->kv = cmp->connected->kv;
    }
    tmp->operation = cmp->operation;
    tmp->kv = cmp->kv;

    if (query.filtersList)
        query.filtersList->compList = tmp;
    else{
        f->compList = tmp;
        query.filtersList = f;
        f = dmalloc(FILTER_SIZE);
        f->next = query.filtersList;
    }

    cmp = cmp->next;
    query.filtersList = f;
}

void setComp(){
    struct fullComparator* tmp = NULL;
    tmp = cmp->next->next;
    cmp->connected = cmp->next;
    cmp->next = tmp;
}

void setCommand(uint8_t command){
    query.command = command;
}

void yyerror (char *s) {fprintf (stderr, "%s\n", s);}