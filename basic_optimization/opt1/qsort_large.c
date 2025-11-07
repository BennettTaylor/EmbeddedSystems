#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define UNLIMIT
#define MAXARRAY 60000 /* this number, if too large, will cause a seg. fault!! */

struct my3DVertexStruct {
  //betaylor changed comparison from distance (double) to squared distance (int) to reduce redundant & expensive computation
  int x, y, z, square_distance;
};

int compare(const void *elem1, const void *elem2)
{
  /* D^2 = (x1 - x2)^2 + (y1 - y2)^2 + (z1 - z2)^2 */
  /* sort based on squared distances from the origin... (same as sorting distances) */
  // Changed comparison to single subtraction
  return (*((struct my3DVertexStruct *)elem1)).square_distance - (*((struct my3DVertexStruct *)elem2)).square_distance;;
}


int
main(int argc, char *argv[]) {
  struct my3DVertexStruct array[MAXARRAY];
  FILE *fp;
  int i,count=0;
  int x, y, z;
  
  if (argc<2) {
    fprintf(stderr,"Usage: qsort_large <file>\n");
    exit(-1);
  }
  else {
    fp = fopen(argv[1],"r");
    
    while((fscanf(fp, "%d", &x) == 1) && (fscanf(fp, "%d", &y) == 1) && (fscanf(fp, "%d", &z) == 1) &&  (count < MAXARRAY)) {
	 array[count].x = x;
	 array[count].y = y;
	 array[count].z = z;
	 //betaylor used squared distance
	 array[count].square_distance = x*x + y*y + z*z;
	 count++;
    }
  }
  printf("\nSorting %d vectors based on distance from the origin.\n\n",count);
  
  qsort(array,count,sizeof(struct my3DVertexStruct),compare);

  for(i=0;i<count;i++)
    printf("%d %d %d\n", array[i].x, array[i].y, array[i].z);
  return 0;
}
