/**
 * Kostra programu pro 3. projekt IZP 2015/16
 *
 * Jednoducha shlukova analyza: 2D nejblizsi soused.
 * Single linkage
 * http://is.muni.cz/th/172767/fi_b/5739129/web/web/slsrov.html
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h> // sqrtf
#include <limits.h> // INT_MAX

/*****************************************************************
 * Ladici makra. Vypnout jejich efekt lze definici makra
 * NDEBUG, napr.:
 *   a) pri prekladu argumentem prekladaci -DNDEBUG
 *   b) v souboru (na radek pred #include <assert.h>
 *      #define NDEBUG
 */
#ifdef NDEBUG
#define debug(s)
#define dfmt(s, ...)
#define dint(i)
#define dfloat(f)
#else

// vypise ladici retezec
#define debug(s) printf("- %s\n", s)

// vypise formatovany ladici vystup - pouziti podobne jako printf
#define dfmt(s, ...) printf(" - "__FILE__":%u: "s"\n",__LINE__,__VA_ARGS__)

// vypise ladici informaci o promenne - pouziti dint(identifikator_promenne)
#define dint(i) printf(" - " __FILE__ ":%u: " #i " = %d\n", __LINE__, i)

// vypise ladici informaci o promenne typu float - pouziti
// dfloat(identifikator_promenne)
#define dfloat(f) printf(" - " __FILE__ ":%u: " #f " = %g\n", __LINE__, f)

#endif

/*****************************************************************
 * Deklarace potrebnych datovych typu:
 *
 * TYTO DEKLARACE NEMENTE
 *
 *   struct obj_t - struktura objektu: identifikator a souradnice
 *   struct cluster_t - shluk objektu:
 *      pocet objektu ve shluku,
 *      kapacita shluku (pocet objektu, pro ktere je rezervovano
 *          misto v poli),
 *      ukazatel na pole shluku.
 */

struct obj_t {
    int id;
    float x;
    float y;
};

struct cluster_t {
    int size;
    int capacity;
    struct obj_t *obj;
};

/*****************************************************************
 * Deklarace potrebnych funkci.
 *
 * PROTOTYPY FUNKCI NEMENTE
 *
 * IMPLEMENTUJTE POUZE FUNKCE NA MISTECH OZNACENYCH 'TODO'
 *
 */

/*
 Inicializace shluku 'c'. Alokuje pamet pro cap objektu (kapacitu).
 Ukazatel NULL u pole objektu znamena kapacitu 0.
*/
void init_cluster(struct cluster_t *c, int cap)
{
    assert(c != NULL);
    assert(cap >= 0);

    if(c == NULL) cap = 0;
    if(cap < 0)
    {
        fprintf(stderr, "Bola zadana nespravna kapacita, cluster preto nema pridelenu ziadnu pamat.\n");
        cap = 0;
    }

    c->obj = malloc(sizeof(struct obj_t) * cap);
    c->capacity = cap;
    c->size = 0;
}

/*
 Odstraneni vsech objektu shluku a inicializace na prazdny shluk.
 */
void clear_cluster(struct cluster_t *c)
{
    if(c->capacity)
    {
        free(c->obj);
    }
    init_cluster(c, 0);
}

/// Chunk of cluster objects. Value recommended for reallocation.
const int CLUSTER_CHUNK = 10;

/*
 Zmena kapacity shluku 'c' na kapacitu 'new_cap'.
 */
struct cluster_t *resize_cluster(struct cluster_t *c, int new_cap)
{
    // TUTO FUNKCI NEMENTE
    assert(c);
    assert(c->capacity >= 0);
    assert(new_cap >= 0);

    if (c->capacity >= new_cap)
        return c;

    size_t size = sizeof(struct obj_t) * new_cap;

    void *arr = realloc(c->obj, size);
    if (arr == NULL)
        return NULL;

    c->obj = arr;
    c->capacity = new_cap;
    return c;
}

/*
 Prida objekt 'obj' na konec shluku 'c'. Rozsiri shluk, pokud se do nej objekt
 nevejde.
 */
void append_cluster(struct cluster_t *c, struct obj_t obj)
{
    if (c->capacity < c->size)
    {
        fprintf(stderr, "Chyba! Kapacita je mensia ako velkost, kluster bude upraveny\n");
        c = resize_cluster(c, c->capacity);
        if(c == NULL)
        {
            fprintf(stderr, "nepodarilo sa realokovat pamat (vo funkcii append_cluster)\n");
            c->capacity = 0;
        }
        c->size = c->capacity;
        return;
    }

    if (c->capacity > c->size)
    {
        c->obj[c->size] = obj;
        c->size ++;
    }
    if (c->capacity == c->size)
    {
        c = resize_cluster(c, c->capacity + 1);
        if(c == NULL)
        {
            fprintf(stderr, "nepodarilo sa realokovat pamat (vo funkcii append_cluster)\n");
            c->size = c->capacity = 0;
            return;
        }
        c->obj[c->size] = obj;
        c->size ++;
    }
}

 /*
 Seradi objekty ve shluku 'c' vzestupne podle jejich identifikacniho cisla.
 */
void sort_cluster(struct cluster_t *c);

/*
 Do shluku 'c1' prida objekty 'c2'. Shluk 'c1' bude v pripade nutnosti rozsiren.
 Objekty ve shluku 'c1' budou serazny vzestupne podle identifikacniho cisla.
 Shluk 'c2' bude nezmenen.
 */
void merge_clusters(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c2 != NULL);

    if(c1 == NULL || c2 == NULL)
    {
        fprintf(stderr, "Chyba! Vo funkcii merge_clusters bol zadany prazdny cluster a preto funkcia zlyhala.\n");
        return;
    }

    for (int i = 0; i < c2->size; i++)
     {
        append_cluster(c1, c2->obj[i]);
     }
    sort_cluster(c1);
}

/**********************************************************************/
/* Prace s polem shluku */

/*
 Odstrani shluk z pole shluku 'carr'. Pole shluku obsahuje 'narr' polozek
 (shluku). Shluk pro odstraneni se nachazi na indexu 'idx'. Funkce vraci novy
 pocet shluku v poli.
*/

int remove_cluster(struct cluster_t *carr, int narr, int idx)
{
    assert(idx < narr);
    assert(narr > 0);

    if(idx > narr)
    {
        fprintf(stderr, "Chyba! Zadany index je vacsi ako pocet prvkov v poli, teda cluster neexistuje a neda sa odstranit\n");
        return -1;
    }

    if(idx < 0)
    {
        fprintf(stderr, "Chyba! Zadany index je mensi ako nula, teda cluster neexistuje a neda sa odstranit)\n");
        return -1;
    }

    if(narr <= 0 || carr == NULL)
    {
        fprintf(stderr, "Chyba! Zadane pole neexistuje alebo je prazdne\n");
        return -1;
    }


    for(int i = idx; i <= narr-1; i++)
    {
        clear_cluster(&carr[i]);
        if(i < narr-1)
        {
         for(int j = 0; j < carr[i+1].size; j++)
            {
             append_cluster(&carr[i], carr[i+1].obj[j]);
            }
        }
    }

    struct cluster_t *arr = resize_cluster(carr, narr-1);
    if(arr == NULL)
    {
        carr = arr;
        fprintf(stderr, "nepodarilo sa alokovat pamat (vo funkcii remove_cluster)");
        return -1;
    }
    else
       {
           carr = arr;
           narr--;
       }
    return narr;
}

/*
 Pocita Euklidovskou vzdalenost mezi dvema objekty.
 */
float obj_distance(struct obj_t *o1, struct obj_t *o2)
{
    assert(o1 != NULL);
    assert(o2 != NULL);

    if( o1 == NULL || o2 == NULL)
    {
        fprintf(stderr, "Jeden z objektov je neinicializovany a preto funkcia obj_distance zlyhala.\n");
        return INT_MAX;
    }

    float prvy = o1->x - o2->x;
    float druhy = o1->y - o2->y;
    prvy = prvy * prvy;
    druhy = druhy * druhy;
   return sqrtf(prvy + druhy);

}

/*
 Pocita vzdalenost dvou shluku. Vzdalenost je vypoctena na zaklade nejblizsiho
 souseda.
*/
float cluster_distance(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c1->size > 0);
    assert(c2 != NULL);
    assert(c2->size > 0);

    if( c1 == NULL || c2 == NULL)
    {
        fprintf(stderr, "Jeden z clustrov je neinicializovany a preto funkcia cluster_distance zlyhala.\n");
        return INT_MAX;
    }

     if(c1->size <= 0 || c2->size <= 0)
    {
        fprintf(stderr, "Jeden z clustro nema ziaden objekt a preto funkcia cluster_distance zlyhala.\n");
        return INT_MAX;
    }

    float nmv = obj_distance(&c1->obj[0], &c2->obj[0]);  //najmensia vzdialenost
    float av = 0.0;  //aktualna vzdialenost
    for(int i = 0; i < c1->size; i++)
    {
        for(int j = 0; j < c2->size; j++)
        {
            av = obj_distance(&c1->obj[i], &c2->obj[j]);
            if(av < nmv) {nmv = av;}
        }
    }
    return nmv;
}

/*
 Funkce najde dva nejblizsi shluky. V poli shluku 'carr' o velikosti 'narr'
 hleda dva nejblizsi shluky (podle nejblizsiho souseda). Nalezene shluky
 identifikuje jejich indexy v poli 'carr'. Funkce nalezene shluky (indexy do
 pole 'carr') uklada do pameti na adresu 'c1' resp. 'c2'.
*/
void find_neighbours(struct cluster_t *carr, int narr, int *c1, int *c2)
{
    assert(narr > 0);

    if(narr <= 0)
    {
        fprintf(stderr, "Pole clustrov je prazdne a preto sa neda najst 2 najblizsie clustre.\n");
        return;
    }

    float av = 0.0;  //aktualna vzdialenost
    float nmv = cluster_distance(&carr[0], &carr[1]);  //najmensia vzdialenost
    for(int i = 0; i < narr; i++)
    {
        for(int j = i+1; j < narr; j++)
        {
            av = cluster_distance(&carr[i], &carr[j]);
            if (av < nmv)
                {
                    nmv = av;
                    *c1 = i;
                    *c2 = j;
                }
        }
    }
}

// pomocna funkce pro razeni shluku
static int obj_sort_compar(const void *a, const void *b)
{
    // TUTO FUNKCI NEMENTE
    const struct obj_t *o1 = a;
    const struct obj_t *o2 = b;
    if (o1->id < o2->id) return -1;
    if (o1->id > o2->id) return 1;
    return 0;
}

/*
 Razeni objektu ve shluku vzestupne podle jejich identifikatoru.
*/
void sort_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE
    qsort(c->obj, c->size, sizeof(struct obj_t), &obj_sort_compar);
}

/*
 Tisk shluku 'c' na stdout.
*/
void print_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE
    for (int i = 0; i < c->size; i++)
    {
        if (i) putchar(' ');
        printf("%d[%g,%g]", c->obj[i].id, c->obj[i].x, c->obj[i].y);
    }
    putchar('\n');
}

/*
 Ze souboru 'filename' nacte objekty. Pro kazdy objekt vytvori shluk a ulozi
 jej do pole shluku. Alokuje prostor pro pole vsech shluku a ukazatel na prvni
 polozku pole (ukalazatel na prvni shluk v alokovanem poli) ulozi do pameti,
 kam se odkazuje parametr 'arr'. Funkce vraci pocet nactenych objektu (shluku).
 V pripade nejake chyby uklada do pameti, kam se odkazuje 'arr', hodnotu NULL.
*/
int load_clusters(char *filename, struct cluster_t **arr)
{
    assert(arr != NULL);

    if( arr == NULL)
    {
        fprintf(stderr, "Chyba! Pole objektov nema ziadnu pamat.\n");
        return 0;
    }

    FILE *subor;
    if((subor=fopen(filename, "r")) == NULL)
    {
    fprintf(stderr, "Chyba pri otvoreni %s\n", filename);
    *arr = NULL;
    return -1;
    }

    unsigned int count;
    char buffer[7];
    fscanf(subor, "%6s %2u", buffer, &count);
    if(count > INT_MAX)
    {
        fprintf(stderr, "Chyba. Pocet objektov je prilis velky. \n");
        return -1;
    }

    *arr = malloc(sizeof(struct cluster_t) * count);
    if(*arr == NULL)
    {
        fprintf(stderr, "zlyhala alokacia pamati \n");
        *arr = NULL;
        return 0;
    }

    unsigned int i;
    struct obj_t objekt;
    for(i = 0; i < count; i++)
    {
      fscanf(subor, "%2d", &objekt.id);
      fscanf(subor, "%3f", &objekt.x);
      fscanf(subor, "%3f", &objekt.y);
      init_cluster(&(*arr)[i], 10);
      append_cluster(&(*arr)[i], objekt);
    }

    if(fclose(subor) == EOF)
    {
        fprintf(stderr, "Chyba pri uzavreni %s\n", filename);
        *arr = NULL;
        return -1;
    }
    return i;
}

/*
 Tisk pole shluku. Parametr 'carr' je ukazatel na prvni polozku (shluk).
 Tiskne se prvnich 'narr' shluku.
*/
void print_clusters(struct cluster_t *carr, int narr)
{
    printf("Clusters:\n");
    for (int i = 0; i < narr; i++)
    {
        printf("cluster %d: ", i);
        print_cluster(&carr[i]);
    }
}

int main(int argc, char *argv[])
{
    if(argc < 2 || argc > 3)
    {
        fprintf(stderr, "bol zadany nespravny pocet argumentov \n");
        return 1;
    }

    char meno_suboru[100];
    strcpy(meno_suboru, argv[1]);

    struct cluster_t *clusters;

    int nacitane_clustre = load_clusters(meno_suboru, &clusters);
    if(nacitane_clustre == -1)
        {
            fprintf(stderr, "funkcia load_cluster zlyhala \n");
            return 1;
        }
    if(nacitane_clustre == 0)
        {
             fprintf(stderr, "Chyba. Nebol nacitany ziadny cluster. \n");
             return 1;
        }

    int pocet_shluku;

    if(argc == 2) pocet_shluku = 1;
     else pocet_shluku = strtol(argv[2], NULL, 10);

    if(pocet_shluku > INT_MAX || pocet_shluku < 1)
        {
            fprintf(stderr, "Chyba. Bol zadany nespravny pocet clustrov \n");
            return 1;
        }

    if(pocet_shluku == nacitane_clustre)
    {
        int i;
        print_clusters(clusters, nacitane_clustre);
        for(i = nacitane_clustre; i > 0; i--)
        {
           nacitane_clustre = remove_cluster(clusters, nacitane_clustre, i-1);
           if(nacitane_clustre == -1)
           {
               fprintf(stderr, "Chyba. Funkcia remove_cluster zlyhala. \n");
               return 1;
           }
        }
        free(clusters);
        return 0;
    }

    int idx1 = 0;
    int idx2 = 0;
    int velkost_pola = nacitane_clustre;
    if(pocet_shluku != nacitane_clustre)

    while (1)
    {
        if (pocet_shluku == velkost_pola)
        {
        int i;
        print_clusters(clusters, velkost_pola);
        for(i = velkost_pola; i > 0; i--)
        {
            velkost_pola = remove_cluster(clusters, velkost_pola, i-1);
            if (velkost_pola == -1)
            {
                fprintf(stderr, "Chyba. Funkcia remove_cluster zlyhala. \n");
                return -1;
            }
        }
        free(clusters);
        return 0;
        }

        find_neighbours(clusters, velkost_pola, &idx1, &idx2);
        merge_clusters(&clusters[idx1], &clusters[idx2]);
        velkost_pola = remove_cluster(clusters, velkost_pola, idx2);
            if (velkost_pola == -1)
            {
            fprintf(stderr, "Chyba. Funkcia remove_cluster zlyhala. \n");
            return 1;
            }
    }

    return 0;
}
