#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct
{
   unsigned char r,g,b;
} Pixel;

typedef struct
{
    double cor;
    int lin,col;
    Pixel C;
}Fereastra;


int padding(int W)
{   int padd;
     if(W%4!=0)
        padd=4-(3*W)%4;
    else
        padd=0;
    return padd;
}


void grayscale_image(char* nume_fisier_sursa,char* nume_fisier_destinatie, int *inaltime_img, int *latime_img)
{
   FILE *fin, *fout;
   unsigned int dim_img;
   unsigned char pRGB[3], header[54], aux;

   fin = fopen(nume_fisier_sursa, "rb");
   if(fin == NULL)
   	{
   		printf("nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fout = fopen(nume_fisier_destinatie, "wb+");

   fseek(fin, 2, SEEK_SET);
   fread(&dim_img, sizeof(unsigned int), 1, fin);

   fseek(fin, 18, SEEK_SET);
   fread(latime_img, sizeof(unsigned int), 1, fin);
   fread(inaltime_img, sizeof(unsigned int), 1, fin);

   //copiaza octet cu octet imaginea initiala in cea noua
	fseek(fin,0,SEEK_SET);
	unsigned char c;
	while(fread(&c,1,1,fin)==1)
	{
		fwrite(&c,1,1,fout);
	}
	fclose(fin);

	//calculam padding-ul pentru o linie
	int padd=padding(latime_img);

	fseek(fout, 54, SEEK_SET);
	int i,j;
	for(i = 0; i < *inaltime_img; i++)
	{
		for(j = 0; j < *latime_img; j++)
		{
			//citesc culorile pixelului
			fread(pRGB, 3, 1, fout);
			//fac conversia in pixel gri
			aux = 0.299*pRGB[2] + 0.587*pRGB[1] + 0.114*pRGB[0];
			pRGB[0] = pRGB[1] = pRGB[2] = aux;
        	fseek(fout, -3, SEEK_CUR);
        	fwrite(pRGB, 3, 1, fout);
        	fflush(fout);
		}
		fseek(fout,padd,SEEK_CUR);
	}
	fclose(fout);
}


double mediapixelifereastra(int **a, int l, int c, int w, int h)
{
    int i,j, S=0,n=w*h;

    for(i=l;i<l+h;i++)
        for(j=c;j<c+w;j++)
            S=S+a[i][j];

    double medie;
    medie=(double)S/(double)n;
    return medie;

}


double deviatia_standard_fereastra(int **a,int l, int c, double fmed, int w,int h)
{
    int i,j, n=w*h;

    double fdev=0.0;

     for(i=l;i<l+h;i++)
        for(j=c;j<c+w;j++)
            fdev=fdev+((double)a[i][j]-fmed)*((double)a[i][j]-fmed);
    fdev=fdev/(double)(n-1);
    return sqrt(fdev);

}


void citire(int ***a, int w, int h, char *imagine)
{
    FILE *fin=fopen(imagine,"rb");

    int padd=padding(w),x,i,j;

    if(fin==NULL)
    {
        printf("nu am gasit imaginea");
        return;
    }

   for(i=0;i<54;i++)
        fread(&x,1,1,fin);

    *a=(int **)malloc(h*sizeof(int*));

    if(*a==NULL)
       {
           printf("Nu s-a gasit suficienta memorie");
           return;
       }

    for(i=h-1;i>=0;i--)
    {
        (*a)[i]=(int *)malloc(w*sizeof(int));
        if((*a)[i]==NULL)
        {
            printf("Nu s-a gasit suficienta memorie");
            return;
        }

        for(j=0;j<w;j++)
        {
            unsigned char Pixeli[3];
            fread(Pixeli,sizeof(unsigned char),3,fin);
            (*a)[i][j]=Pixeli[0];

        }

        fseek(fin,padd,SEEK_CUR);
    }
    fclose(fin);

}


void template_matching(char *imagine, char *imagine_gray, char *sablon, char *sablon_gray, double ps, Fereastra **F ,int *nr, Pixel cul)
{
    int W,H,w,h,i,j,p,q ;

    grayscale_image(imagine,imagine_gray, &H, &W);
    grayscale_image(sablon,sablon_gray,&h,&w);

    int n=w*h;
    int **img, **sab;

    citire(&img,W,H,imagine_gray);
    citire(&sab,w,h,sablon_gray);

    double fmed,Smed, Sdev=0.0, fdev,cor;
    double S=0.0;

    for(i=0;i<h;i++)
           for(j=0;j<w;j++)
            S=S+(double)sab[i][j];

    Smed=S/(double)n;

    for(i=0;i<h;i++)
        for(j=0;j<w;j++)
            Sdev=Sdev+((double)sab[i][j]-Smed)*((double)sab[i][j]-Smed);

    Sdev=Sdev/(double)(n-1);
    Sdev=sqrt(Sdev);

    *F=NULL;
    Fereastra *aux;
    *nr=0;
    for(i=0;i<H-h+1;i++)
        for(j=0;j<W-w+1;j++)
        {
          fmed=mediapixelifereastra(img,i,j,w,h);
          fdev=deviatia_standard_fereastra(img,i,j,fmed,w,h);
          cor=0.0;

          for(p=0;p<h;p++)
            for(q=0;q<w;q++)
               {
                    int l,c;
                    l=i+p;
                    c=j+q;
                    cor=cor+((img[l][c]-fmed)*(sab[p][q]-Smed))/(Sdev*fdev);
               }
            cor=cor/n;
            if(cor>ps)
            {   ++(*nr);
                aux=(Fereastra*)realloc((*F),(*nr)*sizeof(Fereastra));
                if(aux==NULL)
                {
                    printf("Nu exista suficienta memorie");
                    return;
                }
                else
                {
                    (*F)=aux;
                    (*F)[(*nr)-1].lin=i;
                    (*F)[(*nr)-1].col=j;
                    (*F)[(*nr)-1].cor=cor;
                    (*F)[(*nr)-1].C=cul;

                }
            }

        }

        for(i=0;i<H;i++)
            free(img[i]);
        free(img);
        for(i=0;i<h;i++)
            free(sab[i]);
        free(sab);

}


void citire_dimensiuni( char *imagine,int *w, int *h)
{
    int i;

    FILE *fin=fopen(imagine,"rb");
    if(fin==NULL)
    {
        printf("nu am gasit imaginea");
        return;
    }
    fseek(fin, 18, SEEK_SET);
   fread(w, sizeof(unsigned int), 1, fin);
   fread(h, sizeof(unsigned int), 1, fin);
   fclose(fin);


}


void citire_matrice(char *imagine, int w, int h, Pixel ***a, unsigned char **header)
{
    unsigned char x;
    int i,j;

    FILE *fin=fopen(imagine,"rb");
    if(fin==NULL)
    {
        printf("nu am gasit imaginea ");
        return;
    }

    *header=(unsigned char *)malloc(sizeof(unsigned char)*54);
    if(*header==NULL)
    {
        printf("Nu s-a gasit suficienta memorie.");
        return ;
    }

    for(i=0;i<54;i++)
        {
        fread(&x,1,1,fin);
        (*header)[i]=x;
        }

    int padd=padding(w);

    *a=(Pixel **)malloc(h*sizeof(Pixel*));

    if(*a==NULL)
       {
           printf("Nu s-a gasit suficienta memorie");
           return;
       }

    for(i=h-1;i>=0;i--)
    {
        (*a)[i]=(Pixel *)malloc(w*sizeof(Pixel));

        if((*a)[i]==NULL)
        {
            printf("Nu s-a gasit suficienta memorie");
            return;
        }

        for(j=0;j<w;j++)
        {
            unsigned char Pixeli[3];
            fread(Pixeli,sizeof(unsigned char),3,fin);
            (*a)[i][j].b=Pixeli[0];
            (*a)[i][j].g=Pixeli[1];
            (*a)[i][j].r=Pixeli[2];
        }

        fseek(fin,padd,SEEK_CUR);
    }
    fclose(fin);
}


void colorare(char *imagine, Fereastra F, int lat, int lung)
{
    int w,h,i,j;
    Pixel **a;
    unsigned char *header;

    citire_dimensiuni(imagine,&w,&h);
    citire_matrice(imagine,w,h,&a,&header);
    int padd=padding(w);

   FILE *fout=fopen(imagine,"wb");
    if(fout==NULL)
    {
        printf("nu am gasit imaginea");
        return;
    }

    for(j=F.col;j<F.col+lat;j++)

     {
        a[F.lin][j].r=F.C.r;
        a[F.lin][j].g=F.C.g;
        a[F.lin][j].b=F.C.b;
        a[F.lin+lung-1][j].r=F.C.r;
        a[F.lin+lung-1][j].g=F.C.g;
        a[F.lin+lung-1][j].b=F.C.b;
     }

    for(i=F.lin;i<F.lin+lung;i++)
     {  a[i][F.col].r=F.C.r;
        a[i][F.col].g=F.C.g;
        a[i][F.col].b=F.C.b;
        a[i][F.col+lat-1].r=F.C.r;
        a[i][F.col+lat-1].g=F.C.g;
        a[i][F.col+lat-1].b=F.C.b;
     }

     for(i=0;i<54;i++)
        fwrite(&header[i],1,1,fout);

    for(i=h-1;i>=0;i--)
     {
         for(j=0;j<w;j++)
		{

        fwrite(&a[i][j].b,1,1,fout);
        fwrite(&a[i][j].g,1,1,fout);
        fwrite(&a[i][j].r,1,1,fout);
        }
         for(j=0;j<padd;j++)
        {
            int x=0;
            fwrite(&x,1,1,fout);//sărim peste padding
        }

     }

    for(i=0;i<h;i++)
        free(a[i]);
    free(a);

    fclose(fout);
}


void detectii(Fereastra **D, int *n, char *imagine, char *imagine_gray)
{
    *D=NULL;
    *n=0;
    Fereastra *aux, *F;
    Pixel cul;
    FILE *fin=fopen("nume_sabloane.txt","r");
    if(fin==NULL)
    {
        printf("Eroare la deschiderea fisierului");
        return;
    }
    int i,j;
    double ps=0.5;
    char sablon[]="cifra0.bmp";
    char sablon_gray[]="cifra0_gray.bmp";
    for(i=0;i<10;i++)
    {
        if(i==0)
        {
            strcpy(sablon,"cifra0.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra0_gray.bmp");
            cul.r=255;
            cul.g=0;
            cul.b=0;
        }
        if(i==1)
        {
            strcpy(sablon,"cifra1.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra1_gray.bmp");
            cul.r=255;
            cul.g=255;
            cul.b=0;
        }
        if(i==2)
        {
            strcpy(sablon,"cifra2.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra2_gray.bmp");
            cul.r=0;
            cul.g=255;
            cul.b=0;
        }
        if(i==3)
        {
            strcpy(sablon,"cifra3.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra3_gray.bmp");
            cul.r=0;
            cul.g=255;
            cul.b=255;
        }
        if(i==4)
        {
            strcpy(sablon,"cifra4.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra4_gray.bmp");
            cul.r=255;
            cul.g=0;
            cul.b=255;
        }
        if(i==5)
        {
            strcpy(sablon,"cifra5.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra5_gray.bmp");
            cul.r=0;
            cul.g=0;
            cul.b=255;
        }
        if(i==6)
        {
            strcpy(sablon,"cifra6.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra6_gray.bmp");
            cul.r=192;
            cul.g=192;
            cul.b=192;
        }
        if(i==7)
        {
            strcpy(sablon,"cifra7.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra7_gray.bmp");
            cul.r=255;
            cul.g=140;
            cul.b=0;
        }
        if(i==8)
        {
            strcpy(sablon,"cifra8.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra8_gray.bmp");
            cul.r=128;
            cul.g=0;
            cul.b=128;
        }
        if(i==9)
        {
            strcpy(sablon,"cifra9.bmp");
            fscanf(fin,"%s",sablon);
            strcpy(sablon_gray,"cifra9_gray.bmp");
            cul.r=128;
            cul.g=0;
            cul.b=0;
        }

        int nr;
        template_matching(imagine,imagine_gray,sablon,sablon_gray,ps,&F,&nr,cul);

        aux=(Fereastra*)realloc((*D),((*n)+nr)*sizeof(Fereastra));
        if(aux==NULL)
        {
            printf("Nu exista suficienta memorie");
            return;
        }
        else
        {
            (*D)=aux;
            for(j=0;j<nr;j++)
                (*D)[(*n)+j]=F[j];
            (*n)=(*n)+nr;
        }

    free(F);

    }
    fclose(fin);

}


int cmp( const void *a, const void *b)
{
    Fereastra va=*(Fereastra *)a;
    Fereastra vb=*(Fereastra *)b;
    if(va.cor>vb.cor)
        return -1;
    else
        if(va.cor<vb.cor)
        return 1;
    else
        return 0;

}


void sortare(Fereastra *D, int n)
{
    qsort(D,n,sizeof(Fereastra),cmp);
}


int minim(int a, int b)
{
    if(a<b)
        return a;
    else
        return b;
}


int maxim(int a, int b)
{
    if(a>b)
        return a;
    else
        return b;
}


int arie_intersectie(Fereastra a, Fereastra b, int lat, int lung)
{
    if(abs(a.lin-b.lin)>=lat||abs(a.col-b.col)>=lung)
        return 0;
    int l1=minim(a.lin,b.lin);
    int c1=maxim(a.col,b.col);
    int l2=maxim(a.lin+lung-1,b.lin+lung-1);
    int c2=minim(a.col+lat-1,b.col+lat-1);
    int l= abs(l1-l2);
    int L= abs(c1-c2);
    return l*L;
}


int arie_reuniune(Fereastra a, Fereastra b, int lat, int lung)
{
    return 2*lat*lung-arie_intersectie(a,b, lat, lung);
}


void eliminarea_non_maximelor(Fereastra **D, int *n, int lat, int lung)
{
    double suprapunere;
    int i,j,k;

    for(i=0;i<(*n)-1;i++)
        for(j=i+1;j<(*n);j++)
    {
        suprapunere= (double)arie_intersectie((*D)[i],(*D)[j],lat, lung)/(double)arie_reuniune((*D)[i],(*D)[j], lat, lung);

        if(suprapunere>0.2)
        {
            for(k=j+1;k<(*n);k++)
                (*D)[k-1]=(*D)[k];
            (*n)--;
            j--;


          (*D)=(Fereastra*)realloc((*D),(*n)*sizeof(Fereastra));
          if((*D)==NULL)
          {
              printf("Nu s-a gasit suficienta memorie");
              return;
          }


        }
    }
}


void colorare_total( Fereastra *D, int n, char *imagine, int lat, int lung)
{
    int i;
    for(i=0;i<n;i++)
        colorare(imagine, D[i], lat, lung);

}


int main()
{
    Fereastra *D;
    int n,i,lat,lung;

    char imagine[]="test.bmp";
    char imagine_gray[]="test_gray.bmp";
    char sablon[]="cifra0.bmp";
    citire_dimensiuni(sablon,&lat, &lung);

    FILE *fin=fopen("nume_imagini.txt", "r");
    if(fin==NULL)
    {
        printf("eroare la deschiderea fisierului");
        return;
    }

    printf("Numele imaginii sursa este: ");
    fscanf(fin,"%s", imagine);
    printf("%s", imagine);

    printf("\nNumele imaginii pe care se vor desena detectiile sabloanelor este: ");
    fscanf(fin,"%s", imagine_gray);
    printf("%s", imagine_gray);
    printf("\nVom considera cele 10 sabloane aflate in fisier. \nAsteptati...");

    detectii(&D,&n,imagine,imagine_gray);

    sortare(D,n);

    eliminarea_non_maximelor(&D,&n, lat, lung);

    colorare_total(D,n,imagine_gray, lat, lung);

    free(D);
    fclose(fin);

    return 0;
}
