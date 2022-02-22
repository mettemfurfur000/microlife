#include <cstdlib>
#include <fstream>
#include <cmath>
#include <ctime>
#include <ctgmath>
#include <dir.h>

#include <graphics.h>

const int genome_len = 256;
const int size_x = 96;
const int size_y = 160;
const int cellsize = 4;
const int def_lifetime = 256;

int mutation_chance = 10; //5 = 0.5%, 1000 = 100%
int mutation_chance_swap = 20;

int organics2en_k_u = 3; // 3/4 = 0,75
int organics2en_k_d = 4; // 1024e >convert> 1f >convert> 768e

int limit_sim_time = 1000;
int total_sim_time = 0;
int autoreload_population = 0;

int population; //count of cells in current moment

int wind_power = 20; //chance of wind moving
int spawn_chance = 50; //chance of spawn cell in loar/reload

unsigned long long dev_time = 0;

int dev_genomelen_bitset_size = log2(genome_len);

int dev_gen_num = 1;

int sim_id = 0;

int graph = 1;

int lightlevel = 100;

struct quadr
{
	unsigned int gen:4;
};

struct cell
{
	unsigned int id:3;
	
	unsigned int energy:16;
	unsigned int organics:4;
	
	unsigned int gen_select:8;
	
	struct quadr genome[genome_len];
	
	unsigned int lifetime:16;
};

struct cell **plate;

struct cell *genotype;

int light[size_x][size_y];
/*
type
0 - air
1 - organics
2 - organic
3 - cell
4 - dev_wall
*/


int windX = 1024;
int windY = 768;

int linus(FILE *f)
{
	int px=0;
	int py=0;
	int lpx,lpy;
	
	int max = (size_x*size_y);
	
	char str[32] = {0};
	int value;
	int value_num = 0;
	
	int color = COLOR(rand()%256,rand()%256,rand()%256);
	
	int i = 0;
	char c = 0;
	do{
		c = getc(f);
		if(isdigit(c))
		{
			str[i] = c;
			i++;
		}
		if(isspace(c)&&str[0]!=0)
		{
			value = atoi(str);
			px = (float(windX)/(total_sim_time/10)) * value_num;
			py = (float(windY)/max) * value;
			setcolor(color);
			line(lpx,windY-lpy,px,windY-py);
			
			if(value_num%(limit_sim_time/10)==0)
			{
				putpixel(px,0,COLOR(255,0,0));
			}
			
			lpx = px;
			lpy = py;
			value_num++;
			memset(str,0,32);
			i=0;
		}
		if(c==13&&str[0]==0)
		{
			break;
		}
	}while(c!=EOF);
	if(c==EOF)
	{
		return 5;
	}
	return 0;
}

void SaveScreen(int id,int number)
{
	char str[128];
	sprintf(str,"saves\\gen_%d",id);
	mkdir(str);
	sprintf(str,"%s\\pic_%d_%d.bmp",str,id,number);
	writeimagefile(str,0,0,windX-1,windY-1);
}

void Graph(char *path)
{
	FILE * f = fopen(path,"rb");
	while(linus(f)!=5)
	{}
	fclose(f);
}

void wait(int ms)
{
	int CL_PER_MS = CLOCKS_PER_SEC / 1000;
	int waittime = clock() + CL_PER_MS * ms;
	while(waittime > clock()){}
}


int CellSpawn(int x,int y,int type)
{
	if(plate[x][y].id==0)
	{
		plate[x][y].id = type;
		switch(type)
		{
			case 0:plate[x][y].energy = 0; break;
			case 1:plate[x][y].energy = 90; break;
			case 2:plate[x][y].energy = 512; break;
			case 3:plate[x][y].energy = 1024; break;
		}
		switch(type)
		{
			case 0:plate[x][y].organics = 0; break;
			case 1:plate[x][y].organics = 5; break;
			case 2:plate[x][y].organics = 0; break;
			case 3:plate[x][y].organics = 5; break;
		}
		plate[x][y].lifetime = 0;
		if(type==3)
		{
			for(int i=0;i<genome_len;i++)
			{
				plate[x][y].genome[i].gen=rand()%16;
			}
		}else{
			for(int i=0;i<genome_len;i++)
			{
				plate[x][y].genome[i].gen=0;
			}
		}
		return 0;//success
	}
	else
	{
		return 1;//cell not empty >:C
	}
}

void SetAir(int x,int y)
{
	plate[x][y].id = 0;
	plate[x][y].energy = 0;
	plate[x][y].organics = 0;
	plate[x][y].gen_select = 0;
	
	plate[x][y].lifetime = 0;
	
	for(int i=0;i<genome_len;i++)
	{
		plate[x][y].genome[i].gen=0;
	}
}

void SetWall(int x,int y)
{
	plate[x][y].id = 4;
	plate[x][y].energy = 0;
	plate[x][y].organics = 0;
	plate[x][y].gen_select = 0;
	
	plate[x][y].lifetime = 0;
	
	for(int i=0;i<genome_len;i++)
	{
		plate[x][y].genome[i].gen=0;
	}
}

void CellDrop(int x,int y,int type)
{
	int a[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
	int dx,dy;
	int sum=0;
	while(true)
	{
		sum++;
		dx = rand()%3-1;
		dy = rand()%3-1;
		if(a[dx+1][dy+1]==0)
		{
			if(plate[x+dx][y+dy].id==0)
			{
				CellSpawn(x+dx,y+dy,1);
				break;
			}
			else
			{
				if(sum==9) break;
			}
		}
	}
}

int CellConsume(int x,int y,int type)
{
	int loop = 0;
	int sx,sy;
	while(loop<12)
	{
		sx = rand()%3-1;
		sy = rand()%3-1;
		if(plate[x+sx][y+sy].id==type)
		{
			SetAir(x+sx,y+sy);
			plate[x][y].organics++;
			return 0; //success
		}
		loop++;
	}
	return 1; //error, stinky! too crowded!!!
}

struct amo
{
	unsigned int value:8;
};

void CellGenomeCopy(int x,int y,int tx,int ty,int gen_start,int gen_end,int gen_dest)
{
	int buffer[genome_len+50];
	struct amo i;
	int j=0;
	for(i.value=gen_start;i.value!=(gen_end+1)%genome_len;i.value++)
	{
		buffer[j] = plate[x][y].genome[i.value].gen;
		j++;
	}
	
	unsigned int gen_dest_end = j;//calculate size of copied genome
	
	j=0;
	for(i.value=gen_dest;i.value!=(gen_dest_end+1)%genome_len;i.value++)
	{
		plate[tx][ty].genome[i.value].gen = buffer[j];
		j++;
	}
}

void Transfer(int x,int y,int mode, int side)
{
	/*
	modes
	0 - transfer energy (1 to 1)
	1 - transfer organics (1 to 1)
	2 - genome sector (a..b) in c.. (mutatuon chance x2)
	3 - gen selector update
	
	sides
	0 - up
	1 - down
	2 - right
	3 - left
	
	type
	0 - void
	1 - organics
	2 - organic
	3 - cell
	*/	
	//printf("%d|%d|%d|%d\n",x,y,mode,side);
	switch(side)//select side of energy/organics transfer
	{
		case 0:
			if(plate[x][y-1].id==3) //if cell exists
			{
				switch(mode) //select mode of transfer, energy or organics
				{
					case 0:
						if(plate[x][y].energy>512&&plate[x][y-1].energy<65024)
						{
							plate[x][y].energy-=512;
							plate[x][y-1].energy+=512;
						}
						break;
					case 1:
						if(plate[x][y].organics>0&&plate[x][y-1].organics<15)
						{
							plate[x][y].organics-=1;
							plate[x][y-1].organics+=1;
						}
						break;
					case 2:
						if(plate[x][y].energy>256)
						{
							CellGenomeCopy(x,y,x,y-1,plate[x][y].genome[(plate[x][y].gen_select+3)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+4)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+5)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+6)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+7)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+8)%genome_len].gen);
							plate[x][y].energy-=256;
						}
						break;
				}
			}
			break;
		case 1:
			if(plate[x][y+1].id==3)
			{
				switch(mode)
				{
					case 0:
						if(plate[x][y].energy>512)
						{
							plate[x][y].energy-=512;
							plate[x][y+1].energy+=512;
						}
						break;
					case 1:
						if(plate[x][y].organics>0)
						{
							plate[x][y].organics-=1;
							plate[x][y+1].organics+=1;
						}
						break;
					case 2:
						if(plate[x][y].energy>256)
						{
							CellGenomeCopy(x,y,x,y+1,plate[x][y].genome[(plate[x][y].gen_select+3)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+4)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+5)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+6)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+7)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+8)%genome_len].gen);
							plate[x][y].energy-=256;
						}
						break;
				}
			}
			break;
		case 2:
			if(plate[x+1][y].id==3)
			{
				switch(mode)
				{
					case 0:
						if(plate[x][y].energy>512)
						{
							plate[x][y].energy-=512;
							plate[x+1][y].energy+=512;
						}
						break;
					case 1:
						if(plate[x][y].organics>0)
						{
							plate[x][y].organics-=1;
							plate[x+1][y].organics+=1;
						}
						break;
					case 2:
						if(plate[x][y].energy>256)
						{
							CellGenomeCopy(x,y,x+1,y,plate[x][y].genome[(plate[x][y].gen_select+3)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+4)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+5)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+6)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+7)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+8)%genome_len].gen);
							plate[x][y].energy-=256;
						}
						break;
				}
			}
			break;
		case 3:
			if(plate[x-1][y].id==3)
			{
				switch(mode)
				{
					case 0:
						if(plate[x][y].energy>512)
						{
							plate[x][y].energy-=512;
							plate[x-1][y].energy+=512;
						}
						break;
					case 1:
						if(plate[x][y].organics>0)
						{
							plate[x][y].organics-=1;
							plate[x-1][y].organics+=1;
						}
						break;
					case 2:
						if(plate[x][y].energy>256)
						{
							CellGenomeCopy(x,y,x-1,y,plate[x][y].genome[(plate[x][y].gen_select+3)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+4)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+5)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+6)%genome_len].gen,plate[x][y].genome[(plate[x][y].gen_select+7)%genome_len].gen*16 + plate[x][y].genome[(plate[x][y].gen_select+8)%genome_len].gen);
							plate[x][y].energy-=256;
						}
						break;
				}
			}
			break;
	}
}

void CellBite(int x,int y)
{
	unsigned int side = plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen%4;
	/*
	sides
	0 - up
	1 - down
	2 - right
	3 - left
	*/
	int bx=0;
	int by=0;
	switch(side)
	{
		case 0:
			by--;
			break;
		case 1:
			by++;
			break;
		case 2:
			bx++;
			break;
		case 3:
			bx--;
			break;
	}
	
	if(plate[x+bx][y+by].energy>2048&&plate[x][y].energy<64510)
	{
		plate[x+bx][y+by].energy-=2048;
		plate[x][y].energy+=1024;
	}
	if(plate[x+bx][y+by].organics>2&&plate[x][y].organics<15)
	{
		plate[x+bx][y+by].organics-=2;
		plate[x][y].organics+=1;
	}
}

void CellMove(int x,int y,int dx,int dy,int mode)
{
	if(mode == 1 && plate[x][y].id == 3)
	{
		plate[x][y].energy-=4;
	}
	
	//there is nothing interesting
	/*
	if(plate[x][y].stick_up==1&&plate[x][y-1].wait_move==0) 
	{
		plate[x][y].wait_move = 1;
		CellMove(x,y-1,dx,dy,mode);
	}
	if(plate[x][y].stick_down==1&&plate[x][y+1].wait_move==0)
	{
		plate[x][y].wait_move = 1;
		CellMove(x,y+1,dx,dy,mode);
	}
	if(plate[x][y].stick_left==1&&plate[x-1][y].wait_move==0)
	{
		plate[x][y].wait_move = 1;
		CellMove(x-1,y,dx,dy,mode);
	} 
	if(plate[x][y].stick_right==1&&plate[x+1][y].wait_move==0)
	{
		plate[x][y].wait_move = 1;
		CellMove(x+1,y,dx,dy,mode);
	}
	*/
	
	
	//FIRST, move self!
	
	if(plate[x+dx][y+dy].id==1||plate[x+dx][y+dy].id==3||plate[x+dx][y+dy].id==0)
	{
		struct cell buff;
		buff = plate[x][y];
		plate[x][y] = plate[x+dx][y+dy];
		plate[x+dx][y+dy] = buff;
	}
}

void DevErrorsClear()
{
	for(int i = 1;i<size_x-1;i++)
	{
		for(int j = 1;j<size_y-1;j++)
		{
			if(plate[i][j].id>3) SetAir(i,j);
		}
	}
}

int CellClone(int x,int y)
{
	int a[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
	int dx,dy;
	int sum=0;
	while(true)
	{
		dx = rand()%3-1;
		dy = rand()%3-1;
		if(a[dx+1][dy+1]==0)
		{
			if(plate[x+dx][y+dy].id==0)
			{
				goto cellclone;
			}
			else
			{
				sum++;
				if(sum==9) break;
			}
		}
	}
	return 1;
cellclone:
	int swap;
	if(plate[x+dx][y+dy].id==0&&plate[x][y].energy>4096)
	{
		plate[x][y].energy-=3072;
		plate[x+dx][y+dy].id = 3;
		plate[x+dx][y+dy].organics = 0;
		plate[x+dx][y+dy].energy = 1024;
		
		plate[x+dx][y+dy].lifetime = 0;
		
		for(int i=0;i<genome_len;i++)
		{
			plate[x+dx][y+dy].genome[i].gen=plate[x][y].genome[i].gen;
			if(rand()%1000<mutation_chance)
			{
				plate[x+dx][y+dy].genome[i].gen+=rand()%16;
			}
			if(rand()%1000<mutation_chance_swap)
			{
				swap = i+rand()%3-1;
				plate[x+dx][y+dy].genome[i].gen=plate[x][y].genome[swap].gen;
				plate[x+dx][y+dy].genome[swap].gen=plate[x][y].genome[i].gen;
			}
		}
	}
}

void GenomeTick(int x,int y)
{
	plate[x][y].lifetime++;
	int buf;
	int ret;
	int bx=0;
	int by=0;
	int gen = plate[x][y].genome[plate[x][y].gen_select].gen;
	switch(gen)
	{
		//photosintez
		case 1:
			plate[x][y].energy+=light[x][y]*1.5;
			plate[x][y].gen_select++;
			break;
		//organics to energy
		case 2:
			if(plate[x][y].organics>0)
			{
				plate[x][y].organics--;
				plate[x][y].energy+=((1024*organics2en_k_u)/organics2en_k_d);
			}
			plate[x][y].gen_select++;
			break;
		//energy to organics
		case 3:
			if(plate[x][y].energy>1024)
			{
				plate[x][y].organics++;
				plate[x][y].energy-=1024;
			}
			plate[x][y].gen_select++;
			break;
		//organics drop
		case 4:
			if(plate[x][y].organics>0)
			{
				CellDrop(x,y,1);
				plate[x][y].organics--;
			}
			plate[x][y].gen_select++;
			break;
		//organics consume
		case 5:
			if(plate[x][y].organics<15)
			{
				CellConsume(x,y,1);
			}
			plate[x][y].gen_select++;
			break;
		//genome jump
		case 6:
			plate[x][y].gen_select=plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen;
			break;
		//if energy!!!!!!!!!!
		case 7:
			if(plate[x][y].energy/4>plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen+16*plate[x][y].genome[(plate[x][y].gen_select+2)%genome_len].gen)
			{
				plate[x][y].gen_select+=3;
			}else{
				plate[x][y].gen_select+=4;
			}
			break;
		//if organics!!!!!!!!!!!
		case 8:
			if(plate[x][y].organics/4>plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen+16*plate[x][y].genome[(plate[x][y].gen_select+2)%genome_len].gen)
			{
				plate[x][y].gen_select+=3;
			}else{
				plate[x][y].gen_select+=4;
			}
			break;
		case 9:
		//wastes consume & transform to energy
			if(plate[x][y].organics<15)
			{
				CellConsume(x,y,2);
			}
			plate[x][y].gen_select++;
			break;
		case 10:
			break;
		//transfers modes
		case 11:
			Transfer(x,y,plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen%4,plate[x][y].genome[(plate[x][y].gen_select+2)%genome_len].gen%4);
			plate[x][y].gen_select+=7;
			break;
		//bite
		case 12:
			CellBite(x,y);
			plate[x][y].gen_select++;
			break;
		//move
		/*
		sides
		0 - up
		1 - down
		2 - right
		3 - left
		*/
			
		
		case 13:
			switch(plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen%4)
			{
				case 0:
					by--;
					break;
				case 1:
					by++;
					break;
				case 2:
					bx++;
					break;
				case 3:
					bx--;
					break;
			}
			CellMove(x,y,bx,by,1);
			plate[x][y].gen_select+=2;
			break;
		//cell replication
		case 14:
			CellClone(x,y);
			plate[x][y].gen_select++;
			break;
	}
	
	/*
	//dev trash
	
	if(x>10&&x<size_x-10&&y>10&&y<size_y-10)
	{
		if(plate[x+1][y].id==4||plate[x-1][y].id==4||plate[x][y+1].id==4||plate[x][y-1].id==4)
		{
			printf("SUS_gen - %d\n",gen);
		}
	}
	*/
	
}

int CellDraw(int x,int y)
{
	switch(plate[x][y].id)
	{
		case 0:
			setfillstyle(1,COLOR(255,255,255));
			break;
		case 1:
			setfillstyle(1,COLOR(255,176,112));
			break;
		case 2:
			setfillstyle(1,COLOR(181,255,112));
			break;
		case 3:
			setfillstyle(1,COLOR(49,230,0));
			break;
		case 4:
			setfillstyle(1,COLOR(0,0,0));
			break;
		default:
			setfillstyle(1,COLOR(rand()%256,rand()%256,rand()%256));
			break;
	}
	bar(x*cellsize,y*cellsize,x*cellsize+cellsize,y*cellsize+cellsize);
	return 0;
}

void DrawCells()
{
	if(graph!=0)
	{
		for(int i=0;i<size_x;i++)
		{
			for(int j=0;j<size_y;j++)
			{
				CellDraw(i,j);
			}
		}
		swapbuffers();
	}
}

void WindSim();

void WorldTick()
{
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			if(i==0||j==0||i==size_x-1||j==size_y-1) SetWall(i,j);
		}
	}
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			if(plate[i][j].id==3)
			{
				if(plate[i][j].energy>32)//not enough energy? die
				{
					plate[i][j].energy-=32;
				}else{
					SetAir(i,j);
					if(plate[i][j].organics>0)
					{
						CellSpawn(i,j,1);
					}
				}
				
				if(plate[i][j].lifetime>=def_lifetime)//too old? die
				{
					SetAir(i,j);
					if(plate[i][j].organics>0)
					{
						CellSpawn(i,j,1);
					}
				}
				GenomeTick(i,j);
			}
		}
	}
}

void LightGen(int power)
{
	int buff[size_x][size_y];
	int b;
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			b = rand()%100 + size_y - j + power-100;
			if(b<0) b = 0;
			light[i][j]=b;
		}
	}
	int num=0;
	int t=1;
	for(int k=0;k<7;k++)//smooth0
	{
		for(int x=0;x<size_x;x++)
		{
			for(int y=0;y<size_y;y++)
			{
				num=0;
				t=1;
				num+=light[x][y];
				int wsxl = size_x-1;
				int wsyl = size_y-1;
				if(x>0)					{num+=light[x-1][y];	t++;}
				if(y>0)					{num+=light[x][y-1];	t++;}
				if(x<wsxl)				{num+=light[x+1][y];	t++;}
				if(y<wsyl)				{num+=light[x][y+1];	t++;}
				if(x>0&&y>0)			{num+=light[x-1][y-1];	t++;}
				if(x<wsxl&&y>0)			{num+=light[x+1][y-1];	t++;}
				if(y<wsyl&&x>0)			{num+=light[x-1][y+1];	t++;}
				if(x<wsxl&&y<wsyl)		{num+=+light[x+1][y+1];	t++;}
				buff[x][y]=int(num/t);
			}
		}
		for(int x=0;x<size_x;x++) 
		{
			for(int y=0;y<size_y;y++) 
			{
				light[x][y]=buff[x][y];
			}
		}
	}
}

struct raw_cell
{
	unsigned char data[300];
};



void FCellPrint(FILE * file,struct cell source)
{
	fprintf(file,"[ %d %d %d %d %d ",
	source.id,
	source.energy,
	source.organics,
	source.gen_select,
	source.lifetime
	);
	for(int i=0;i<genome_len;i++)
	{
		fprintf(file,"%d ",source.genome[i].gen);
	}
	fprintf(file,"]\n");
}

void FCellPaster(struct cell *dest,int pos,char *str)
{
	int value = atoi(str);
	switch(pos)
	{
		case 0:
			dest->id = value;
			break;
		case 1:
			dest->energy = value;
			break;
		case 2:
			dest->organics = value;
			break;
		case 3:
			dest->gen_select = value;
			break;
		case 4:
			dest->lifetime = value;
			break;
	}
}

void FCellRead(FILE * file,struct cell *dest)
{
	char str[128];
	memset(str,0,128);
	char c;
	int i=0;
	int variable=0;
	//read variables of cell: id, energy, and other
    do {
    	if(variable==5) break;
    	c = getc(file);
    	if(isdigit(c))
    	{
    		str[i]=c;
    		i++;
		}
		if(c==' '&&str[0]!=0)
		{
			FCellPaster(dest,variable,str);
			memset(str,0,128);
			i=0;
			variable++;
		}
	}
	while(c!=EOF);
	
	//read cell genome
	i=0;
	variable=0;
	do {
    	c = getc(file);
    	if(isdigit(c))
    	{
    		str[i]=c;
    		i++;
		}
		if(c==' '&&str[0]!=0)
		{
			dest->genome[variable].gen = atoi(str);
			memset(str,0,128);
			i=0;
			variable++;
		}
	}
	while(c!=']');
}

void Save(int id,int number)
{
	char str[128];
	
	sprintf(str,"saves\\gen_%d",id);
	mkdir(str);
	sprintf(str,"%s\\savefile%d_%d.bin",str,number,population);
	
	FILE * f = fopen(str,"wb");
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			FCellPrint(f,plate[i][j]);
		}
	}
	fclose(f);
}

void Load()
{
	FILE * f = fopen("savefile.bin","rb");
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			FCellRead(f,&plate[i][j]);
		}
	}
	fclose(f);
}

void LightShow()
{
	swapbuffers();
	int l = 0;
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			l = light[i][j];
			setfillstyle(1,COLOR(l,l,l));
			bar(i*cellsize,j*cellsize,i*cellsize+cellsize,j*cellsize+cellsize);
		}
	}
	swapbuffers();
}

void TextShow(char *str,int time,int dx,int dy)
{
	int x_center = size_x/2 + dx;
	int y_center = size_y/2 + dy;
	
	int c_l = strlen(str)+1;
	int wind_x = c_l + 1;
	int wind_y = 4;
	swapbuffers();
	setfillstyle(4,COLOR(132,99,99));
	bar((x_center-wind_x)*cellsize,(y_center-wind_y)*cellsize,(x_center+wind_x)*cellsize,(y_center+wind_y)*cellsize);

	setfillstyle(1,COLOR(199,162,111));
	bar((x_center-wind_x+1)*cellsize,(y_center-wind_y+1)*cellsize,(x_center+wind_x-1)*cellsize,(y_center+wind_y-1)*cellsize);
	
	outtextxy((x_center-wind_x+2)*cellsize,(y_center-wind_y+2)*cellsize,str);
	swapbuffers();
	wait(time);
}

void Clean()
{
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			SetAir(i,j);
		}
	}
}

void GetText(char *dest,char *request);

void SetWalls();

void StatPrint(char *message);

void RandomCellSpawn(int chance);

void CreateGraphicAndReload(int id,int gen);

void Reload()
{
	
	lightlevel = 100;
	LightGen(lightlevel);
	dev_gen_num = 0;
	dev_time = 0;
	sim_id++;
	CreateGraphicAndReload(sim_id,dev_gen_num);
	Clean();
	SetWalls();
	RandomCellSpawn(spawn_chance);
	StatPrint("\n\n");
	total_sim_time = 0;
	system("del stats.bin");
}

void NewCycle()
{
	lightlevel--;
	LightGen(lightlevel);
	dev_gen_num++;
	dev_time = 0;
}

void GetText(char *dest,char *request)
{
	char command[128] = {0};
	char c;
	int i = 0;
	TextShow(request,5,0,0);
	while(true)
	{
		c = getch();
		if(c==13) break; //if ENTER
		if(c==8&&i>0)
		{
			i--;
			command[i] = 0;
		}
		else
		{
			command[i] = c;
			i++;
		}
		TextShow(command,5,0,8);
		if(i>=128)
		{
			TextShow("Too long command!",500,0,0);
			break;
		}
	}
	strcpy(dest,command);
}

void CommandInput(char *str);

void CommandProcessor()
{
	char command[256] = {0};
	if(kbhit()!=0&&getch()=='c')
	{
		GetText(command,"Print Command");
	}
	CommandInput(command);
}

void StatCellNum(int dev_time)
{
	FILE * f = fopen("stats.bin","a");
	int cells = 0;
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			if(plate[i][j].id==3)
			{
				cells++;
			}
		}
	}
	
	population = cells;
	
	char str[32];
	sprintf(str,"%d \t",cells);
	fputs(str,f);
	fclose(f);
}

void StatPrint(char *message)
{
	FILE * f = fopen("stats.bin","a");
	fputs(message,f);
	fclose(f);
}

void SetWalls()
{
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			if(i==0||j==0||i==size_x-1||j==size_y-1) SetWall(i,j);
		}
	}
}

void PlateInit()
{
	plate = new struct cell *[size_x];
	for(int i=0;i<size_x;i++)
	{
		plate[i] = new struct cell [size_y];
		for(int j=0;j<size_y;j++)
		{
			SetAir(i,j);
			if(i==0||j==0||i==size_x-1||j==size_y-1) SetWall(i,j);
		}
	}
}

void RandomCellSpawn(int chance)
{
	int real_chance = int((1.0/chance)*100);
	for(int i=0;i<size_x;i++)
	{
		srand(rand()+rand());
		for(int j=0;j<size_y;j++)
		{
			if(rand()%real_chance==0)
			{
				CellSpawn(i,j,3);
			}
		}
	}
}

void WindSim()
{
	int real_power = 100.0/wind_power;
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			if(rand()%real_power==0&&plate[i][j].id!=4&&plate[i][j].id!=0)
			{
				CellMove(i,j,rand()%3-1,rand()%3-1,0);
			}
			if(rand()%real_power==0&&plate[i][j].id==1)
			{
				CellMove(i,j,rand()%3-1,1,0);
			}
		}
	}
}

void CreateGraphicAndReload(int id,int gen)
{
	closegraph();
	
	//////////////////////////////
	initwindow(windX,windY);
	
	Graph("stats.bin");
	
	SaveScreen(id,gen);
	
	closegraph();
	//////////////////////////
	
	initwindow(size_x*cellsize,size_y*cellsize,"CyberBiology v0.1");
	swapbuffers();
}

void CommandInput(char *str)
{
	char command[128] = {0};
	if(strcmp(str,"set")==0)
	{
		GetText(command,"Print Sub-Command");
		if(strcmp(command,"mut-a")==0)
		{
			memset(command,0,128);
			GetText(command,"Print Value");
			mutation_chance = atoi(command);
		}
		if(strcmp(command,"mut-b")==0)
		{
			memset(command,0,128);
			GetText(command,"Print Value");
			mutation_chance_swap = atoi(command);
		}
		if(strcmp(command,"cell-chance")==0)
		{
			memset(command,0,128);
			GetText(command,"Print Value");
			spawn_chance = atoi(command);
		}
		if(strcmp(command,"autoreload-time")==0)
		{
			memset(command,0,128);
			GetText(command,"Print Value");
			spawn_chance = atoi(command);
		}
	}
	if(strcmp(str,"save")==0)
	{
		Save(0,0);
		TextShow("Plate Saved! Check file savefile0.bin",500,0,0);
	}
	if(strcmp(str,"load")==0)
	{
		Load();
		TextShow("Plate Loaded!",500,0,0);
	}
	if(strcmp(str,"light")==0)
	{
		TextShow("Showing LightMap. . .",500,0,0);
		LightShow();
		getch();
	}
	if(strcmp(str,"exit")==0)
	{
		StatPrint("\n\n");
		system("del stats.bin");
		exit(0);
	}
	if(strcmp(str,"reload")==0)
	{
		Reload();
	}
	if(strcmp(str,"grph-toggle")==0)
	{
		if(graph==1)
		{
			TextShow("GRAPHICS TOGGLE TO OFF",1000,0,0);
			graph = 0;
		}else{
			TextShow("GRAPHICS TOGGLE TO ON",1000,0,0);
			graph = 1;
		}
	}
}

int main()
{
	mkdir("saves");
	srand(time(NULL));
	initwindow(size_x*cellsize,size_y*cellsize,"CyberBiology v0.1");
	outtextxy(10,10,"Loading...");
	
	PlateInit();
	RandomCellSpawn(spawn_chance);
	StatPrint("\n");
	LightGen(lightlevel);
	swapbuffers();
	char str[128];
	while(true)
	{
		if(population<=autoreload_population)
		{
			Reload();
		}
		if(dev_time>=limit_sim_time)
		{
			lightlevel--;
			//Save(sim_id,dev_gen_num);
			NewCycle();
		}
		if(kbhit()!=0&&getch()=='c')
		{
			GetText(str,"Print Command");
			CommandInput(str);
		}
		if(dev_time%10==0)
		{
			DevErrorsClear();
			DrawCells();
			printf("%d\n",total_sim_time);
			StatCellNum(dev_time);
			WindSim();
		}
		WorldTick();
		dev_time++;
		total_sim_time++;
	}
	StatPrint("\n\n");
	getch();
	return 0;
}
