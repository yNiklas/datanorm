#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Datanorm 3: https://www.kommunal-edv.de/wissen/it-technik/schnittstellen/datanorm/
// Datanorm 5: https://docplayer.org/115761786-Technische-spezifikationen-der-datanorm-dateien-in-haufe-lexware.html

typedef struct Product
{
    /*
    Format:
    datatype name // Datensatz-Spalte
    */
    char* artNr; // A-2 or B-2
    char* name; // A-4+A-5 or T[0]-6
    char* longName1; // A-5
    char* operationSign; // A-1 or B-1

    // in Datanorm: 1=Brutto, 2=Netto
    char* isPriceExclVAT; // A-6 or P-3

    // 0 = price for one piece
    // 1 = price for 10 pieces
    // 2 = price for 100 pieces
    // 3 = price for 1000 pieces
    uint8_t priceMeasure; // A-7

    // Unit of measure (Mengeneinheit Stück, Liter, Stunden, ...)
    char* measure; // A-8

    uint32_t price; // A-9 or P-4

    // Refers to the RAB file
    char* discountGroup; // A-10

    // Refers to the WRG file
    char* articleGroup; // A-11

    // Identifies the corresponding long text in the T dataset
    char* longTextKey; // A-12

    char* matchcode; // B-3
    char* alternativeArtNr; // B-4
    uint32_t catalogPage; // B-5
    uint16_t cuIdentifier; // B-7
    uint32_t weight; // B-8
    char* ean; // B-9

    char* longTexts; // All T-6 entries where the longTextKey matches
} Product;

typedef struct PList
{
    char* artNr;
    char* longTextKey;
    Product* product;
    struct PList* next;
} PList;

typedef struct {
    const char *start;
    size_t len;
} token;

char **split(const char *str, char sep)
{
    char **array;
    unsigned int start = 0, stop, toks = 0, t;
    token *tokens = malloc((strlen(str) + 1) * sizeof(token));
    for (stop = 0; str[stop]; stop++) {
        if (str[stop] == sep) {
            tokens[toks].start = str + start;
            tokens[toks].len = stop - start;
            toks++;
            start = stop + 1;
        }
    }
    /* Mop up the last token */
    tokens[toks].start = str + start;
    tokens[toks].len = stop - start;
    toks++;
    array = malloc((toks + 1) * sizeof(char*));
    for (t = 0; t < toks; t++) {
        /* Calloc makes it nul-terminated */
        char *token = calloc(tokens[t].len + 1, 1);
        memcpy(token, tokens[t].start, tokens[t].len);
        array[t] = token;
    }
    /* Add a sentinel */
    array[t] = NULL; 
    free(tokens);
    return array;
}

size_t stringlength(const char *s)
{
    if (s == NULL) return 0;
    
    size_t len = 0;
    while (s[len++] != '\0');

    return len - 1;
}

void escapeSpecialChars(char** line, size_t len) {
    char x = (*line)[0];
    char* lineToNow = malloc(len * sizeof(char));
    size_t newLength = len;
    size_t lineToNowIdx = 0;

    for (int i = 0; x!='\0'; i++) {
        x = (*line)[i];
        if (x<0) {
            char* replaceStr = "ü";
            switch (x) {
                case -127: replaceStr = "ü"; break;
                case -124: replaceStr = "ä"; break;
                case -108: replaceStr = "ö"; break;
                case -31: replaceStr = "ß"; break;
                case -103: replaceStr = "Ö"; break;
                case -102: replaceStr = "Ü"; break;
                case -114: replaceStr = "Ä"; break;
                default: break;
            }

            char* newLineToNow = malloc(++newLength);
            strcpy(newLineToNow, lineToNow);
            strcpy(lineToNow, newLineToNow);

            lineToNow[lineToNowIdx++] = replaceStr[0];
            lineToNow[lineToNowIdx++] = replaceStr[1];
        } else {
            lineToNow[lineToNowIdx++] = x;
        }
    }
    *line = lineToNow;
}

size_t arraylength(char** array) {
    size_t len = 0;
    while (array[len] != NULL) {
        len++;
    }
    
    return len;
}

void initPListItem(PList* item) {
    item->artNr = "";
    item->longTextKey = "";
    item->product = malloc(sizeof(Product));
    item->next = NULL;

    item->product->artNr = "";
    item->product->name = "";
    item->product->longName1 = "";
    item->product->operationSign = "";
    item->product->isPriceExclVAT = "";
    item->product->priceMeasure = 0;
    item->product->measure = "";
    item->product->discountGroup = "";
    item->product->articleGroup = "";
    item->product->longTextKey = "";
    item->product->matchcode = "";
    item->product->alternativeArtNr = "";
    item->product->catalogPage = 0;
    item->product->cuIdentifier = 0;
    item->product->weight = 0;
    item->product->ean = "";
    item->product->longTexts = "";
}

void build_T_Product(PList* item, char** tset, uint8_t init) {
    if (init != 0) {
        initPListItem(item);
    }
    
    if (item->product->name == NULL || stringlength(item->product->name) == 0) {
        item->product->name = tset[6];
        item->product->longName1 = tset[9];
    } else {
        if (item->product->longTexts == NULL || stringlength(item->product->longTexts) == 0) {
            item->product->longTexts = strcat(tset[6], strcat(tset[9], " "));
        } else {
            item->product->longTexts =
                strcat(item->product->longTexts, strcat(tset[6], strcat(tset[9], " ")));
        }
    }
    item->product->longTextKey = tset[2];
    item->longTextKey = tset[2];
}

PList* check_T_Set(char** line, PList* pList) {
    char** tset = split(*line, ';');
    
    if (arraylength(tset) != 11) {
        printf("PARSE ERROR %s", *line);
    }

    uint8_t found = 0;
    PList* item = pList;
    while (item != NULL) {
        if (item->longTextKey != NULL && strcmp(item->longTextKey, tset[2]) == 0) {
            build_T_Product(item, tset, 0);
            found = 1;
            break;
        }
        item = item->next;
    }
    
    if (found == 0) {
        PList* newPListItem = malloc(sizeof(PList));
        build_T_Product(newPListItem, tset, 1);
        return newPListItem;
    }
    return NULL;
}

void build_A_Product(PList* item, char** aset, uint8_t init) {
    if (init != 0) {
        initPListItem(item);
    }
    
    if (item->artNr == NULL || stringlength(item->artNr) == 0) {
        item->artNr = aset[2];
        item->product->artNr = aset[2];
    }
    
    if (item->product->operationSign == NULL || stringlength(item->product->operationSign) == 0) {
        item->product->operationSign = aset[1];
    }
    
    if (item->product->name == NULL || stringlength(item->product->name) == 0) {
        item->product->name = strcat(aset[4], strcat(aset[5], " "));
    }

    if (item->product->longName1 == NULL || stringlength(item->product->longName1) == 0) {
        item->product->longName1 = aset[5];
    }
    
    if (aset[6] != NULL && stringlength(aset[6]) > 0) {
        item->product->isPriceExclVAT = aset[6];
    }
    
    if (aset[7] != NULL && stringlength(aset[7]) > 0) {
        item->product->priceMeasure = atoi(aset[7]);
    }
    
    if (item->product->price <= 0) {
        item->product->price = atoi(aset[9]);
    }
    
    if (item->product->discountGroup == NULL || stringlength(item->product->discountGroup) == 0) {
        item->product->discountGroup = aset[10];
    }
    
    if (item->product->articleGroup == NULL || stringlength(item->product->articleGroup) == 0) {
        item->product->discountGroup = aset[11];
    }
    
    if (item->longTextKey <= 0) {
        item->product->longTextKey = aset[12];
    }
}

PList* check_A_Set(char** line, PList* pList) {
    char** aset = split(*line, ';');

    if (arraylength(aset) != 14) {
        printf("PARSE ERROR %s", *line);
    }

    char* artNr = aset[2];
    if (artNr == 0 || stringlength(artNr) == 0) {
        printf("WARN: A-Set line without Article Number");
        return NULL;
    }

    uint8_t found = 0;
    PList* item = pList;
    while (item != NULL) {
        if ((item->artNr != NULL && strcmp(item->artNr, artNr) == 0) ||
              (item->longTextKey != NULL && strcmp(aset[12], item->longTextKey))) {
            build_A_Product(item, aset, 0);
            found = 1;
            break;
        }
        item = item->next;
    }
    
    if (!found) {
        PList* newPListItem = malloc(sizeof(PList));
        build_A_Product(newPListItem, aset, 1);
        return newPListItem;
    }
    return NULL;
}

char* parseString(char* str) {
    return str == NULL ? "" : str;
}

void writeToFile(PList* items) {
    FILE* fp = fopen("output.txt", "w");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }

    fprintf(fp, "%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\n",
        "ArtNr", "Name", "Langname", "Verarbeitungszeichen", "Preiskennzeichen",
        "Preiseinheit", "Mengeneinheit", "Preis", "Rabattgruppe", "Artikelgruppe", "Langtextschlüssel",
        "Matchcode", "Alternative ArtNr", "Katalogseite", "Kupfer-Kennzahl", "Kupfergewicht", "EAN",
        "Zusatzinformationen");

    PList* item = items;
    while (item != NULL) {
        if (item->product == NULL) {
            item = item->next;
            continue;
        }
        printf("%s\n", item->artNr);

        fprintf(fp, "%s;%s;%s;%s;%s;%d;%s;%d;%s;%s;%s;%s;%s;%d;%d;%d;%s;%s\n",
            parseString(item->artNr),
            parseString(item->product->name),
            parseString(item->product->longName1),
            parseString(item->product->operationSign),
            parseString(item->product->isPriceExclVAT),
            item->product->priceMeasure,
            parseString(item->product->measure),
            item->product->price,
            parseString(item->product->discountGroup),
            parseString(item->product->articleGroup),
            parseString(item->product->longTextKey),
            parseString(item->product->matchcode),
            parseString(item->product->alternativeArtNr),
            item->product->catalogPage,
            item->product->cuIdentifier,
            item->product->weight,
            parseString(item->product->ean),
            parseString(item->product->longTexts));
        item = item->next;
    }

    fclose(fp);
}

int main() {
    FILE* fp = fopen("datanorm.001.html", "r");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }

    PList* pList = malloc(sizeof(PList));
    pList->next = NULL;
    pList->product = NULL;

    char* line = NULL;
    size_t len = 0;
    uint32_t counter = 0;
    while (getline(&line, &len, fp) != -1 && counter < 4000) {
        escapeSpecialChars(&line, len);

        char setId = line[0];
        PList* newlyCreated = NULL;
        if (setId == 'T') {
            newlyCreated = check_T_Set(&line, pList);
        } else if (setId == 'A') {
            newlyCreated = check_A_Set(&line, pList);
        }

        if (newlyCreated != NULL) {
            newlyCreated->next = pList;
            pList = newlyCreated;
        }

        counter++;
    }
    fclose(fp);
    if (line) free(line);

    writeToFile(pList);
}
