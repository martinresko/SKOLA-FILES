// Martin Rešovský, 29.11.5015,   xresov00, druhy projekt, itineracni vypocty. Program implementuje taylorov vzorec a zretazene zlomky na vypocet logaritmu, taktiez zistuje pocet nutnych iteracii na uistenie dostatocnej presnosti.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

//funkcia ktora nahradza knihovnu funkciu fabs a vytvara absolutnu hodnotu cisla
double myfabs(double x)
{
    return x < 0 ? -x : x;
}

//funkcia ktora implementuje taylorov vzorec na vypocet logaritmu dvoma sposobmy - pre cisla mensie ako jeden a vacsie ako jeden.
double taylor_log(double x, unsigned int n)
{
    if (x == INFINITY) return INFINITY;
    if (x == INFINITY) return NAN;
    if (x == 0) return -INFINITY;
    if (x < 0) return NAN;
    if (n <= 0) return 0;
    if (x > 0 && x <= 1)
    {
        x = 1.0 - x;
        double s = -x;
        double t = -x;
        double k = 2;
        while(k <= n)
        {
            t = t * x;
            s = s + (t / k);
            k++;
        }
        return s;
    }
    else
    {
        double i = 1.0;
        double s = 0.0;
        double t = 1.0;
        while(i <= n)
        {
            t *= ((x-1) / x);
            s = s + (t / i);
            i++;
        }
        return s;
    }
}

//funkcia ktora implementuje vzorec zretazeneho zlomku na vypocet logaritmu
double  cfrac_log(double x, unsigned int n)
{
    if (x == INFINITY) return INFINITY;
    if (x == -INFINITY) return NAN;
    if (x == 0) return -INFINITY;
    if (x < 0) return NAN;
    if (n <= 0) return 0;
    x = ((x - 1.0) / (x + 1.0));
    double step = 0.0;
    n = (double) n;
    n = n-1;
    while (n >= 1.0)
    {
        step =  (n*n) * (x*x) / ( ( 2 * n + 1.0) - step );
        n--;
    }
  return (2.0 * x) / (1.0 - step);
}

//funkcia ktora pocita pocet iteracii pre dosiahnutie dostatocne presneho vysledku. Vyuziva funkciu talor_log
double taylor_log_iter(double x, double eps)
{
 if (eps <= 0) return 0;
 unsigned int pi = 1;
    while(myfabs( taylor_log(x, pi) - log(x) ) > eps)
          {
              pi++;
          }
    return pi;
}

//funkcia ktora pocita pocet iteracii pre dosiahnutie dostatocne presneho vysledku. Vyuziva funkciu cfrac_log
double cfrac_log_iter(double x, double eps)
{
    if (eps <= 0) return 0;
    unsigned int pi = 1;
    while(myfabs( cfrac_log(x, pi) - log(x) ) > eps)
          {
              pi++;
          }
    return pi;
}

int main(int argc, char **argv)
{
   bool vypocet_logaritmu = false;
   bool vypocet_iteracii = false;

   if (argc == 4 && (strcmp(argv[1], "--log") == 0)) vypocet_logaritmu = true;
   if (argc == 5 && (strcmp(argv[1], "--iter") == 0)) vypocet_iteracii = true;
   if (vypocet_logaritmu == false && vypocet_iteracii == false)
    {
     fprintf(stderr, "Chyba! Boli zadane nevhodne argumenty.");
        return 1;
    }

   if (vypocet_logaritmu == true)
   {

     char *endptr1;
     char *endptr2;
     double x = strtod(argv[2], &endptr1);
     int n = strtod(argv[3], &endptr2);

     if((x == NAN) && (strlen(endptr1) != 0)) {fprintf(stderr, "Chyba! Boli zadane nevhodne argumenty."); return 1;}
     if(n <= 0) {fprintf(stderr, "Chyba! Boli zadane nevhodne argumenty."); return 1;}

     printf("log(%.4g) = %.12g\n", x, log(x));
     printf("cf_log(%.4g) = %.12g\n", x, cfrac_log(x,n));
     printf("taylor_log(%.4g) = %.12g\n", x, taylor_log(x, n));
   }

   if (vypocet_iteracii == true)
   {
     char *endptr1;
     char *endptr2;
     char *endptr3;
     double min = strtod(argv[2], &endptr1);
     double max = strtod(argv[3], &endptr2);
     double eps = strtod(argv[4], &endptr3);

     if((min == NAN) && (strlen(endptr2) != 0)) {fprintf(stderr, "Chyba! Boli zadane nevhodne argumenty."); return 1;}
     if((max == NAN) && (strlen(endptr2) != 0)) {fprintf(stderr, "Chyba! Boli zadane nevhodne argumenty."); return 1;}
     if(eps <= 0) {fprintf(stderr, "Chyba! Boli zadane nevhodne argumenty."); return 1;}

     printf("log(%.4g) = %.12g\n",min, log(min));
     printf("log(%.4g) = %.12g\n",max, log(max));

     unsigned int a = cfrac_log_iter(min, eps);
     unsigned int b = cfrac_log_iter(max, eps);
     unsigned int npi; //najvyssi pocet iteracii
     if(a > b)
     {
         printf("continued fraction iterations = %d\n",a);
         npi = a;
     }
     else
     {
         printf("continued fraction iterations = %d\n",b);
         npi = b;
     }
     printf("cf_log(%.4g) = %.12g\n", min, cfrac_log(min, npi));
     printf("cf_log(%.4g) = %.12g\n", max, cfrac_log(max, npi));

     unsigned int c = taylor_log_iter(min, eps);
     unsigned int d = taylor_log_iter(max, eps);
     if(c > d)
     {
         printf("taylor polynomial iterations = %d\n",c);
         npi = c;
     }
     else
     {
         printf("taylor polynomial iterations = %d\n",d);
         npi = d;
     }
     printf("taylor_log(%.4g) = %.12g\n", min, taylor_log(min, npi));
     printf("taylor_log(%.4g) = %.12g\n", max, taylor_log(max, npi));
   }
   return(0);
}
