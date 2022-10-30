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

    uint8_t discountTypeA; // P-5
    char* discountA; // P-6

    uint8_t discountTypeB; // P-7
    char* discountB; // P-8

    uint8_t discountTypeC; // P-9
    char* discountC; // P-10
} Product;

typedef struct PList
{
    char* artNr;
    char* longTextKey;
    Product* product;
    struct PList* next;
} PList;



/*
 Utility
*/
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

void dump(PList* pList) {
    PList* item = pList;
    while (item!=NULL) {
        if (item->product != NULL) {
            printf("%s %s\n", item->longTextKey, item->product->name);
        }
        item = item->next;
    }
}

size_t arraylength(char** array) {
    size_t len = 0;
    while (array[len] != NULL) {
        len++;
    }
    
    return len;
}

char* ccpy(char* origin) {
    char* new = malloc(stringlength(origin)+1);
    strcpy(new, origin);
    return new;
}

char* concat(char* s1, char* s2, char* s3) {
    uint8_t offset = 2;
    if (stringlength(s1) == 0 && stringlength(s2) == 0) {
        offset = 1;
    }

    char* total = malloc(stringlength(s1)+stringlength(s2)+stringlength(s3)+offset);
    strcpy(total, s1);
    strcat(total, s2);
    if (offset == 2) strcat(total, " ");
    strcat(total, s3);
    return total;
}

void freeSet(char** set) {
    size_t i = 0;
    while (set[i] != NULL) {
        free(set[i]);
        i++;
    }
}


/*
 Line pre-processing
*/
void escapeSpecialChars(char** line, size_t len) {
    char x = (*line)[0];
    char* lineToNow = malloc(len < 240 ? 240 : len);
    size_t newLength = len;
    size_t lineToNowIdx = 0;
    uint8_t delimsHave = 0;
    uint8_t delimsShould = 0;

    if (x == 'A') {
        delimsShould = 13;
    } else if (x == 'T') {
        delimsShould = 10;
    }

    for (int i = 0; i<len&&(x!='\0'||delimsHave<delimsShould); i++) {
        x = (*line)[i];
        if (x<0) {
            char* replaceStr = "ü";
            switch (x) {
                case -83:
                case -127: replaceStr = "ü"; break;
                case -124: replaceStr = "ä"; break;
                case -108: replaceStr = "ö"; break;
                case -82:
                case -31: replaceStr = "ß"; break;
                case -103: replaceStr = "Ö"; break;
                case -102: replaceStr = "Ü"; break;
                case -114: replaceStr = "Ä"; break;
                case -8: replaceStr = "°"; break;
                case -99: replaceStr = "Ø"; break;
                case -77: replaceStr = "³"; break;
                case -3: replaceStr = "²"; break;
                default: printf("unknown %d %s\n", x, *line); break;
            }

            char* newLineToNow = malloc(++newLength);
            strncpy(newLineToNow, lineToNow, newLength);
            strncpy(lineToNow, newLineToNow, len < 240 ? 240 : len);

            lineToNow[lineToNowIdx++] = replaceStr[0];
            lineToNow[lineToNowIdx++] = replaceStr[1];
        } else {
            if (x == ';') {
                delimsHave++;
            }
            if (x == '\0') {
                // lineToNow[lineToNowIdx++] = ' ';
            } else {
                lineToNow[lineToNowIdx++] = x;
            }
        }
    }

    char* finalLine = malloc(lineToNowIdx);
    strncpy(finalLine, lineToNow, lineToNowIdx);
    free(lineToNow);
    for (size_t i = 0; i < lineToNowIdx-1; i++) {
        if (finalLine[i] == '\0') {
            finalLine[i] = ' ';
        }
    }
    finalLine[lineToNowIdx-1] = '\0';
    *line = finalLine;
}



/*
 List item creation
*/
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
    item->product->discountTypeA = 0;
    item->product->discountTypeB = 0;
    item->product->discountTypeC = 0;
    item->product->discountA = "";
    item->product->discountB = "";
    item->product->discountC = "";
}



/*
 Set processing
*/
void build_T_Product(PList* item, char** tset, uint8_t init) {
    if (init != 0) {
        initPListItem(item);
    }

    if (item->product->longTexts == NULL || stringlength(item->product->longTexts) == 0) {
        item->product->longTexts = concat(item->product->longTexts, tset[6], tset[9]);
    } else {
        char* total = malloc(stringlength(item->product->longTexts)+stringlength(tset[6])+stringlength(tset[9])+3);
        strcpy(total, item->product->longTexts);
        strcat(total, " ");
        strcat(total, tset[6]);
        strcat(total, " ");
        strcat(total, tset[9]);
        free(item->product->longTexts);
        item->product->longTexts = total;
    }

    char* tset2 = ccpy(tset[2]);
    item->product->longTextKey = tset2;
    item->longTextKey = tset2;
}

PList* check_T_Set(char** line, PList* pList) {
    char** tset = split(*line, ';');
    
    if (arraylength(tset) != 11) {
        printf("PARSE ERROR %s\n", *line);
        return NULL;
    }

    uint8_t found = 0;
    PList* item = pList;
    while (item != NULL) {
        if (item->longTextKey != NULL && strcmp(item->longTextKey, tset[2]) == 0) {
            build_T_Product(item, tset, 0);
            found = 1;
        }
        item = item->next;
    }
    
    if (found == 0) {
        PList* newPListItem = malloc(sizeof(PList));
        build_T_Product(newPListItem, tset, 1);
        freeSet(tset);
        free(tset);
        return newPListItem;
    }

    freeSet(tset);
    free(tset);

    return NULL;
}

void build_A_Product(PList* item, char** aset, uint8_t init) {
    if (init != 0) {
        initPListItem(item);
    }
    
    if (item->artNr == NULL || stringlength(item->artNr) == 0) {
        char* aset2 = ccpy(aset[2]);
        item->artNr = aset2;
        item->product->artNr = aset2;
    }
    
    if (item->product->operationSign == NULL || stringlength(item->product->operationSign) == 0) {
        item->product->operationSign = ccpy(aset[1]);
    }
    
    if (item->product->name == NULL || stringlength(item->product->name) == 0 || strcmp(item->product->name, " ") == 0) {
        item->product->name = concat(item->product->name, aset[4], aset[5]);
    }

    if (item->product->longName1 == NULL || stringlength(item->product->longName1) == 0) {
        item->product->longName1 = ccpy(aset[5]);
    }
    
    if (aset[6] != NULL && stringlength(aset[6]) > 0) {
        item->product->isPriceExclVAT = ccpy(aset[6]);
    }
    
    if (aset[7] != NULL && stringlength(aset[7]) > 0) {
        item->product->priceMeasure = atoi(aset[7]);
    }
    
    if (item->product->price <= 0) {
        item->product->price = atoi(aset[9]);
    }
    
    if (item->product->discountGroup == NULL || stringlength(item->product->discountGroup) == 0) {
        item->product->discountGroup = ccpy(aset[10]);
    }
    
    if (item->product->articleGroup == NULL || stringlength(item->product->articleGroup) == 0) {
        item->product->discountGroup = ccpy(aset[11]);
    }
    
    if (stringlength(item->longTextKey) == 0) {
        char* aset12 = ccpy(aset[12]);
        item->product->longTextKey = aset12;
        item->longTextKey = aset12;
    }
}

PList* check_A_Set(char** line, PList* pList) {
    char** aset = split(*line, ';');

    if (arraylength(aset) != 14) {
        printf("PARSE ERROR %s\n", *line);
        return NULL;
    }

    char* artNr = aset[2];
    if (artNr == 0 || stringlength(artNr) <= 1) {
        printf("WARN: A-Set line without Article Number\n");
        return NULL;
    }

    uint8_t found = 0;
    PList* item = pList;
    while (item != NULL) {
        if (item->artNr == NULL && item->longTextKey == NULL) {
            item = item->next;
            continue;
        }

        if (stringlength(aset[12]) > 0 && strcmp(aset[12], item->longTextKey) == 0) {
            if (strcmp(item->artNr, artNr) == 0 || stringlength(item->artNr) == 0) {
                build_A_Product(item, aset, 0);
                found = 1;
                break;
            } else {
                // Multiple products using this longTextKey
                PList* copiedNewItem = malloc(sizeof(PList));
                build_A_Product(copiedNewItem, aset, 1);
                copiedNewItem->product->longTexts = item->product->longTexts;
                if (stringlength(copiedNewItem->product->name) == 0 || strcmp(copiedNewItem->product->name, "") == 0) {
                    copiedNewItem->product->name = item->product->name;
                }
                if (stringlength(copiedNewItem->product->longName1) == 0) {
                    copiedNewItem->product->longName1 = item->product->longName1;
                }
                freeSet(aset);
                free(aset);
                return copiedNewItem;
            }
        }
        item = item->next;
    }
    
    if (found == 0) {
        PList* newPListItem = malloc(sizeof(PList));
        build_A_Product(newPListItem, aset, 1);
        freeSet(aset);
        free(aset);
        return newPListItem;
    }

    freeSet(aset);
    free(aset);

    return NULL;
}

void build_P_Product(PList* item, char** adjustedPset, uint8_t init) {
    if (init != 0) {
        initPListItem(item);
    }

    if (stringlength(item->artNr) == 0) {
        item->artNr = adjustedPset[0];
        item->product->artNr = adjustedPset[0];
    }

    item->product->isPriceExclVAT = ccpy(adjustedPset[1]);
    item->product->price = atoi(adjustedPset[2]);
    item->product->discountTypeA = atoi(adjustedPset[3]);
    item->product->discountA = adjustedPset[4];
    item->product->discountTypeB = atoi(adjustedPset[3]);
    item->product->discountB = adjustedPset[4];
    item->product->discountTypeC = atoi(adjustedPset[3]);
    item->product->discountC = adjustedPset[4];
}

PList* check_Single_P_Set(char** pset, PList* pList) {
    uint8_t found = 0;
    PList* item = pList;
    while (item != NULL) {
        if (item->artNr == NULL) {
            item = item->next;
            continue;
        }

        if (strcmp(item->artNr, pset[0]) == 0) {
            build_P_Product(item, pset, 0);
            found = 1;
            break;
        }
        item = item->next;
    }
    
    if (found == 0) {
        PList* newItem = malloc(sizeof(PList));
        build_P_Product(newItem, pset, 1);
        return newItem;
    }

    return NULL;
}

PList* check_P_Set(char** line, PList* pList) {
    char** pset = split(*line, ';');

    if (arraylength(pset) < 12) {
        printf("PARSE ERROR %s\n", *line);
        return NULL;
    }

    pset += 2;
    if (arraylength(pset) % 9 != 1) {
        printf("P-PARSE ERROR (2) %s\n", *line);
        return NULL;
    }

    PList* newItems = NULL;
    while (arraylength(pset) > 1 && arraylength(pset) % 9 == 1) {
        PList* newItem = check_Single_P_Set(pset, pList);
        if (newItem != NULL) {
            if (newItems != NULL) {
                newItem->next = newItems;
            }
            newItems = newItem;
        }

        pset += 9;
    }

    return newItems;
}


/*
 File writings
*/
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

int main(int argc, char* argv[]) {
    PList* pList = malloc(sizeof(PList));
    pList->next = NULL;
    pList->product = NULL;

    for (int i = 1; i < argc; i++) {
        FILE* fp = fopen(argv[i], "r");
        if (fp == NULL) {
            exit(EXIT_FAILURE);
        }

        char* line = NULL;
        size_t len = 0;
        while (getline(&line, &len, fp) != -1) {
            escapeSpecialChars(&line, len);

            char setId = line[0];
            PList* newlyCreated = NULL;
            if (setId == 'T') {
                newlyCreated = check_T_Set(&line, pList);
            } else if (setId == 'A') {
                newlyCreated = check_A_Set(&line, pList);
            } else if (setId == 'P') {
                newlyCreated = check_P_Set(&line, pList);
            }

            if (newlyCreated != NULL) {
                if (newlyCreated->next == NULL) {
                    newlyCreated->next = pList;
                    pList = newlyCreated;
                } else {
                    PList* lastOfNew = newlyCreated->next;
                    while (lastOfNew->next != NULL) {
                        lastOfNew = lastOfNew->next;
                    }
                    lastOfNew->next = pList;
                    pList = newlyCreated;
                }
            }

            free(line);
            len = 0;
        }

        fclose(fp);
        if (line) free(line);
    }
    
    writeToFile(pList);
}
