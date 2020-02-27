#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
   unsigned char r,g,b;
} Pixel;

int padding(int W)
{   int padd;
     if(W%4!=0)
        padd=4-(3*W)%4;
    else
        padd=0;
    return padd;
}
void citire_img_liniarizata(char* nume_fisier_sursa, Pixel **Vector,int **header, int *H, int *W)
{
    int dim;
    FILE *fin;
    fin = fopen(nume_fisier_sursa, "rb");

    if(fin == NULL)
   	{
   		printf("nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fseek(fin, 2, SEEK_SET);
   fread(&dim, sizeof(unsigned int), 1, fin);

   fseek(fin, 18, SEEK_SET);
   fread(*(&W), sizeof(unsigned int), 1, fin);
   fread(*(&H), sizeof(unsigned int), 1, fin);

    int padd;
    padd=padding(*W);

    *header=(int *)malloc(sizeof(int)*54);
    if((*header)==NULL)
    {
        printf("Nu s-a gasit suficienta memorie.");
        return ;
    }

    int i,x;
    fseek(fin, 0, SEEK_SET);
    for(i=0;i<54;i++)
    {
        fread(&x,1,1,fin);
        (*header)[i]=x;
    }


    *Vector=(Pixel*)malloc(sizeof(Pixel)*dim/3);
    if((*Vector)==NULL)
    {
        printf("Nu s-a gasit suficienta memorie");
        return;
    }
    Pixel P;
    int j;

	for(i=0;i<*H;i++)
	{
		for(j=0;j<*W;j++)
		{
			//citesc culorile pixelului
        char Pixeli[3];
        int poz;
        poz=(*W)*(*H)-(*W)*(i+1)+j;// liniarizam matricea rasturnata (prima linie o punem ultima in vector)
        fread(Pixeli,sizeof(char),3,fin);
        P.r = Pixeli[2];
        P.g = Pixeli[1];
        P.b = Pixeli[0];
        (*Vector)[poz]=P;
        }
        fseek(fin,padd,SEEK_CUR);//sarim peste padding
    }
    fclose(fin);
}

void afisare_imagine(char *nume_fisier_iesire, Pixel *Vector,int *header, int H, int W)
{
    FILE *fout;
    fout=fopen(nume_fisier_iesire,"wb");

    int i,j,padd;
    padd=padding(W);
    for(i=0;i<54;i++)
        fwrite(&header[i],1,1,fout);
    for(i=0;i<H;i++)
	{

	    for(j=0;j<W;j++)
		{
        int poz;
        poz=W*H-W*(i+1)+j;
        fwrite(&Vector[poz].b,1,1,fout);
        fwrite(&Vector[poz].g,1,1,fout);
        fwrite(&Vector[poz].r,1,1,fout);
        }

        for(j=0;j<padd;j++)
        {
            int x=0;
            fwrite(&x,1,1,fout);//sărim peste padding
        }

	}
	fclose(fout);
	free(header);
}

void XORSHIFT32 (unsigned int **R, unsigned int R0, int W, int H)
{
    unsigned int i,n;
    unsigned int r=R0;
    n=2*W*H-1;
    *R=(unsigned int*)malloc(n*sizeof(unsigned int));
    if((*R)==NULL)
    {
        printf("Nu s-a gasit suficienta memorie");
        return;
    }
    for(i=1;i<=n;i++)
    {
        r=r^r<<13;
        r=r^r>>17;
        r=r^r<<5;
        *(*R+i)=r;
    }

}

void permutare_aleatoare( unsigned int **R, unsigned int R0, int W, int H)
{
    XORSHIFT32(R, R0, W,H);
}

void Durstenfeld(unsigned int **P, unsigned int *R, int W, int H)
{
    unsigned int n= W*H,r,aux;
    int i;
    *P=(unsigned int *)malloc(n*sizeof(unsigned int));
    if((*P)==NULL)
    {
        printf("Nu s-a gasit suficienta spatiu");
        return;
    }
    for(i=0;i<n;i++)
   {
       (*P)[i]=i;

   }
    for(i=n-1;i>=1;i--)
    {
        r=R[n-i]%(i+1);
        aux=(*P)[r];
        (*P)[r]=(*P)[i];
        (*P)[i]=aux;
    }
}
void imagine_intermediara(Pixel **Vector2, Pixel *Vector, unsigned int *P,int W, int H)
{
    (*Vector2)=(Pixel *)malloc(W*H*sizeof(Pixel));
    if((*Vector2)==NULL)
    {
        printf("Nu s-a gasit suficienta memorie");
        return;
    }

    int i;
    for(i=0;i<W*H;i++)
    {
        (*Vector2)[P[i]]=Vector[i];
    }
}

void criptare(char *nume_img_sursa, char *nume_img_iesire, const char *key, Pixel **C)
{
    int i, H, W, *header;
    Pixel *Vector,*Vector2;//vector=imagine liniarizata, vector2=imaginea intermediara permutata;

    unsigned int R0,SV,*R, *P;//R= secventa de numere intregi aleatoare, P=permutarea aleatoare
    unsigned char *sv;
    unsigned char *r;

    FILE *fin=fopen(key,"r");
    if(fin==NULL)
    {
        printf("Nu am gasit cheia secreta");
        return;
    }

    fscanf(fin,"%u",&R0);
    fscanf(fin,"%u",&SV);

	citire_img_liniarizata(nume_img_sursa,&Vector,&header,&H,&W);
	permutare_aleatoare(&R,R0,W,H);

    Durstenfeld(&P,R,W,H);
    imagine_intermediara(&Vector2, Vector,P,W,H);

    sv=(unsigned char *)&SV;
    r=(unsigned char *)&R[W*H];

    (*C)=(Pixel *)malloc(W*H*sizeof(Pixel));

    (*C)[0].b=sv[0]^Vector2[0].b^r[0];
    (*C)[0].g=sv[1]^Vector2[0].g^r[1];
    (*C)[0].r=sv[0]^Vector2[0].r^r[2];
    for(i=1;i<W*H;i++)
    {   r=(unsigned char *)&R[W*H+i];
        (*C)[i].b=(*C)[i-1].b^Vector2[i].b^r[0];
        (*C)[i].g=(*C)[i-1].g^Vector2[i].g^r[1];
        (*C)[i].r=(*C)[i-1].r^Vector2[i].r^r[2];

    }

    afisare_imagine(nume_img_iesire,*C,header,H,W);
    free(Vector);
    free(Vector2);
    free(R);
    free(P);
    fclose(fin);
}

void inversa(unsigned int *P, unsigned int **P1, int W, int H )
{
    int n=W*H,i;
    (*P1)=(unsigned int *)malloc(n*sizeof(int));
    if((*P1)==NULL)
    {
        printf("Nu s-a gasit suficienta memorie");
        return;
    }
    for(i=0;i<n;i++)
        (*P1)[P[i]]=i;
}

void decriptare(char *nume_img_sursa, char *nume_img_iesire, const char *key)
{
    int i, H, W, *header;
    Pixel *Vector,*Vector2, *C, *C1, *D;//vector=imagine liniarizata, vector2=imaginea intermediara permutata,
                                // C=imaginea criptata, C1=imagine criptata intermediara, D=imaginea decriptata;

    unsigned int R0,SV,*R, *P, *P1;//R= secventa de numere intregi aleatoare, P=permutarea aleatoare,P1=permutarea inversa
    unsigned char *sv;
    unsigned char *r;
    char img_decriptat[]="peppersdecriptat.bmp";
    printf("numele imaginii decriptate= ");
    scanf("%s",img_decriptat);
    printf("\n");

    criptare(nume_img_sursa,nume_img_iesire,key, &C);

    FILE *fin=fopen(key,"r");
    if(fin==NULL)
    {
        printf("Nu am gasit cheia secreta!!!  ");
        perror(key);
        return;
    }


    fscanf(fin,"%u",&R0);
    fscanf(fin,"%u",&SV);

	citire_img_liniarizata(nume_img_sursa,&Vector,&header,&H,&W);

	permutare_aleatoare(&R,R0,W,H);
    Durstenfeld(&P,R,W,H);
    inversa (P,&P1,W,H);

    sv=(unsigned char *)&SV;
    r=(unsigned char *)&R[W*H];

    C1=(Pixel *)malloc(W*H*sizeof(Pixel));
    if(C1==NULL)
    {
        printf("Nu s-a gasit suficienta memorie");
        return;
    }

    C1[0].b=sv[0]^C[0].b^r[0];
    C1[0].g=sv[1]^C[0].g^r[1];
    C1[0].r=sv[0]^C[0].r^r[2];
    for(i=1;i<W*H;i++)
    {   r=(unsigned char *)&R[W*H+i];
        C1[i].b=C[i-1].b^C[i].b^r[0];
        C1[i].g=C[i-1].g^C[i].g^r[1];
        C1[i].r=C[i-1].r^C[i].r^r[2];
    }

    imagine_intermediara(&D,C1,P1,W,H);

    afisare_imagine(img_decriptat,D,header,H,W);

    free(Vector);
    free(Vector2);
    free(C);
    free(R);
    free(P);
    free(P1);
    free(D);
    free(C1);
    fclose(fin);
}

void chipatrat( char *imagine)
{
    double r,g,b,f;
    r=g=b=0.0;
    int *fr,*fg,*fb,i;
    fr=(int *)calloc(256,sizeof(int));
    fg=(int *)calloc(256,sizeof(int));
    fb=(int *)calloc(256,sizeof(int));
    if(fr==NULL||fg==NULL||fb==NULL)
    {
        printf("Nu s-a gasit suficienta memorie");
        return;
    }

    int W,H;
    FILE *fin;
    fin = fopen(imagine, "rb");

    if(fin == NULL)
   	{
   		printf("nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fseek(fin, 18, SEEK_SET);
   fread(&H, sizeof(unsigned int), 1, fin);
   fread(&W, sizeof(unsigned int), 1, fin);

    int padd;
    padd=padding(W);

    int x;
    fseek(fin, 0, SEEK_SET);
    for(i=0;i<54;i++)
        fread(&x,1,1,fin);

    Pixel P;
    int j;

    f=(float)H*W/256.0;

	for(i=0;i<H;i++)
	{
		for(j=0;j<W;j++)
		{

        char Pixeli[3];
        fread(Pixeli,sizeof(char),3,fin);
        P.r = Pixeli[2];
        P.g = Pixeli[1];
        P.b = Pixeli[0];
        fr[P.r]++;
        fg[P.g]++;
        fb[P.b]++;

        }
        fseek(fin,padd,SEEK_CUR);
    }
    for(i=0;i<256;i++)
      {
        r=r+(fr[i]-f)*(fr[i]-f)/f;
        g=g+(fg[i]-f)*(fg[i]-f)/f;
        b=b+(fb[i]-f)*(fb[i]-f)/f;
      }

	printf("\nChi-squared test on RGB channels for: %s \nRosu: %.2lf   Verde:%.2lf  Albastru: %.2lf\n",imagine,r,g,b);
    fclose(fin);
}

int main()
{

    char nume_img_sursa[] = "peppers.bmp";

    printf("numele imaginii sursa = ");
    scanf("%s",nume_img_sursa);
    printf("\n");

	char nume_img_iesire[] = "pepperscriptat.bmp";

	printf("numele imaginii criptate = ");
	scanf("%s",nume_img_iesire);
	printf("\n");

	const char key[]="secret_key.txt";

	printf("numele fisierului ce contine cheia secreta = ");
	scanf("%s",key);
	printf("\n");

    decriptare(nume_img_sursa,nume_img_iesire,key);

    chipatrat(nume_img_sursa);
    chipatrat(nume_img_iesire);

    return 0;
}
