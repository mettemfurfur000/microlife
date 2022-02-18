#include <cstdlib>
#include <fstream>
#include <cmath>
#include <ctime>
#include <ctgmath>

#include <graphics.h>

const int genome_len = 256;
const int size_x = 128;
const int size_y = 128;
const int cellsize = 4;

int mutation_chance = 10; //5 = 0.5%, 1000 = 100%
int fat2en_k_u = 3;
int fat2en_k_d = 4;

/*
if top var is 3, and bottom var is 4, fat2en_k = 3/4 = 0.75;

why not single float variable?

i just multiple on k_u and divide on k_d - thats easier for our processor <3
*/

int dev_genomelen_bitset_size = log2(genome_len);

struct quadr
{
	unsigned int gen:4;
};

struct cell
{
	unsigned int id:3;
	
	unsigned int energy:16;
	unsigned int fat:4;
	
	unsigned int gen_select:8;
	
	struct quadr genome[genome_len];
	
	unsigned int stick_up:1;
	unsigned int stick_down:1;
	unsigned int stick_left:1;
	unsigned int stick_right:1;
	
	unsigned int wait_move:1;
};

struct cell **plate;

int light[size_x][size_y];
/*
type
0 - air
1 - fat
2 - organic
3 - cell
4 - dev_wall
*/

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
			case 0:plate[x][y].fat = 0; break;
			case 1:plate[x][y].fat = 5; break;
			case 2:plate[x][y].fat = 0; break;
			case 3:plate[x][y].fat = 5; break;
		}
		plate[x][y].stick_up = 0;
		plate[x][y].stick_down = 0;
		plate[x][y].stick_left = 0;
		plate[x][y].stick_right = 0;
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
		plate[x][y].wait_move = 0;
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
	plate[x][y].fat = 0;
	plate[x][y].gen_select = 0;
	
	plate[x][y].stick_up = 0;
	plate[x][y].stick_down = 0;
	plate[x][y].stick_left = 0;
	plate[x][y].stick_right = 0;
	
	plate[x][y].wait_move = 0;
	
	for(int i=0;i<genome_len;i++)
	{
		plate[x][y].genome[i].gen=0;
	}
}

void SetWall(int x,int y)
{
	plate[x][y].id = 4;
	plate[x][y].energy = 0;
	plate[x][y].fat = 0;
	plate[x][y].gen_select = 0;
	
	plate[x][y].stick_up = 0;
	plate[x][y].stick_down = 0;
	plate[x][y].stick_left = 0;
	plate[x][y].stick_right = 0;
	
	plate[x][y].wait_move = 0;
	
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
CellDropLable:
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
		else goto CellDropLable;
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
		if(plate[x+sx][y+sy].id==1)
		{
			SetAir(x+sx,y+sy);
			plate[x][y].fat++;
			return 0; //success
		}
		loop++;
	}
	return 1; //error, stinky! too crowded!!!
}

void CellChekStick(int x,int y)
{
	if(plate[x+1][y].id==0) plate[x][y].stick_left = 0;
	if(plate[x-1][y].id==0) plate[x][y].stick_right = 0;
	if(plate[x][y+1].id==0) plate[x][y].stick_down = 0;
	if(plate[x][y-1].id==0) plate[x][y].stick_up = 0;
}

void CellStick(int x,int y,int side,int flag)
{
	switch(side)
	{
		case 0:
			if(plate[x][y+1].id!=4)
			{
				plate[x][y].stick_up = flag;
				plate[x][y+1].stick_down = flag;
			}
			break;
		case 1:
			if(plate[x][y-1].id!=4)
			{
				plate[x][y].stick_down = flag;
				plate[x][y-1].stick_up = flag;
			}
			break;
		case 2:
			if(plate[x+1][y].id!=4)
			{
				plate[x][y].stick_right = flag;
				plate[x+1][y].stick_left = flag;
			}
			break;
		case 3:
			if(plate[x-1][y].id!=4)
			{
				plate[x][y].stick_left = flag;
				plate[x-1][y].stick_right = flag;
			}
			break;
	}
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
	1 - transfer fat (1 to 1)
	2 - genome sector (a..b) in c.. (mutatuon chance x2)
	3 - gen selector update
	
	sides
	0 - up
	1 - down
	2 - right
	3 - left
	
	type
	0 - void
	1 - fat
	2 - organic
	3 - cell
	*/	
	//printf("%d|%d|%d|%d\n",x,y,mode,side);
	switch(side)//select side of energy/fat transfer
	{
		case 0:
			if(plate[x][y+1].id==3) //if cell exists
			{
				switch(mode) //select mode of transfer, energy or fat
				{
					case 0:
						if(plate[x][y].energy>512&&plate[x][y+1].energy<65024)
						{
							plate[x][y].energy-=512;
							plate[x][y+1].energy+=512;
						}
						break;
					case 1:
						if(plate[x][y].fat>0&&plate[x][y+1].fat<15)
						{
							plate[x][y].fat-=1;
							plate[x][y+1].fat+=1;
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
		case 1:
			if(plate[x][y-1].id==3)
			{
				switch(mode)
				{
					case 0:
						if(plate[x][y].energy>512)
						{
							plate[x][y].energy-=512;
							plate[x][y-1].energy+=512;
						}
						break;
					case 1:
						if(plate[x][y].fat>0)
						{
							plate[x][y].fat-=1;
							plate[x][y-1].fat+=1;
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
						if(plate[x][y].fat>0)
						{
							plate[x][y].fat-=1;
							plate[x+1][y].fat+=1;
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
						if(plate[x][y].fat>0)
						{
							plate[x][y].fat-=1;
							plate[x-1][y].fat+=1;
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
			bx--;
			break;
		case 3:
			bx++;
			break;
	}
	
	if(plate[x+bx][y+by].energy>512)
	{
		plate[x+bx][y+by].energy-=512;
		plate[x][y].energy+=128;
	}
	if(plate[x+bx][y+by].fat>2)
	{
		plate[x+bx][y+by].fat-=2;
		plate[x][y].energy+=1;
	}
}

void CellMove(int x,int y,int dx,int dy,int mode)
{
	if(mode == 1 && plate[x][y].id == 3)
	{
		plate[x][y].energy-=4;
	}
	
	//first, move others (if you sticky)
	
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
	
	//second, move self! (if free)
	
	if(plate[x+dx][y+dy].id==0)
	{
		struct cell buff;
		buff = plate[x][y];
		plate[x][y] = plate[x+dx][y+dy];
		plate[x+dx][y+dy] =  buff;
	}
	plate[x][y].wait_move = 0;
}

void CellClone(int x,int y,int dx,int dy)
{
	if(plate[x+dx][y+dy].id==0&&plate[x][y].energy>2048)
	{
		plate[x][y].energy-=2048;
		plate[x+dx][y+dy].id = 3;
		plate[x+dx][y+dy].fat = 0;
		plate[x+dx][y+dy].energy = 1024;
		
		plate[x+dx][y+dy].gen_select = 0;
		
		plate[x+dx][y+dy].stick_up = 0;
		plate[x+dx][y+dy].stick_down = 0;
		plate[x+dx][y+dy].stick_left = 0;
		plate[x+dx][y+dy].stick_right = 0;
		
		for(int i=0;i<genome_len;i++)
		{
			plate[x+dx][y+dy].genome[i].gen=plate[x][y].genome[i].gen;
			if(rand()%1000<mutation_chance)
			{
				plate[x+dx][y+dy].genome[i].gen+=rand()%16;
			}
		}
	}
}

void GenomeTick(int x,int y)
{
	int buf;
	int ret;
	int bx=0;
	int by=0;
	int gen = plate[x][y].genome[plate[x][y].gen_select].gen;
	switch(gen)
	{
		//photosintez
		case 1:
			plate[x][y].energy+=light[x][y]*3;
			plate[x][y].gen_select++;
			break;
		//fat to energy
		case 2:
			if(plate[x][y].fat>0)
			{
				plate[x][y].fat--;
				plate[x][y].energy+=((1024*fat2en_k_u)/fat2en_k_d);
			}
			plate[x][y].gen_select++;
			break;
		//energy to fat
		case 3:
			if(plate[x][y].energy>1024)
			{
				plate[x][y].fat++;
				plate[x][y].energy-=1024;
			}
			plate[x][y].gen_select++;
			break;
		//fat drop
		case 4:
			if(plate[x][y].fat>0)
			{
				CellDrop(x,y,1);
				plate[x][y].fat--;
			}
			plate[x][y].gen_select++;
			break;
		//fat consume
		case 5:
			if(plate[x][y].fat<15)
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
		//if fat!!!!!!!!!!!
		case 8:
			if(plate[x][y].fat/4>plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen+16*plate[x][y].genome[(plate[x][y].gen_select+2)%genome_len].gen)
			{
				plate[x][y].gen_select+=3;
			}else{
				plate[x][y].gen_select+=4;
			}
			break;
		//sticky man
		case 9:
			CellStick(x,y,plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen%4,1);
			plate[x][y].gen_select++;
			break;
		//not sticky man
		case 10:
			CellStick(x,y,plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen%4,0);
			plate[x][y].gen_select++;
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
					bx--;
					break;
				case 3:
					bx++;
					break;
			}
			CellMove(x,y,by,bx,1);
			plate[x][y].gen_select+=2;
			break;
		//cell replication
		case 14:
			switch(plate[x][y].genome[(plate[x][y].gen_select+1)%genome_len].gen%4)
			{
				case 0:
					by--;
					break;
				case 1:
					by++;
					break;
				case 2:
					bx--;
					break;
				case 3:
					bx++;
					break;
			}
			CellClone(x,y,bx,by);
			plate[x][y].gen_select+=2;
			break;
	}
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
	}
	bar(x*cellsize,y*cellsize,x*cellsize+cellsize,y*cellsize+cellsize);
	return 0;
}

void DrawCells()
{
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			CellDraw(i,j);
		}
	}
}

void WorldTick()
{
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			if(plate[i][j].id==3)
			{
				CellChekStick(i,j);
				if(plate[i][j].energy>32)
				{
					plate[i][j].energy-=32;
				}else{
					SetAir(i,j);
					if(plate[i][j].fat>0)
					{
						CellSpawn(i,j,1);
					}
				}
				GenomeTick(i,j);
			}
		}
	}
}

void LightGen()
{
	int buff[size_x][size_y];
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			light[i][j]=rand()%100;
		}
	}
	int num=0;
	int t=1;
	for(int k=0;k<20;k++)//smooth0
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

void CellConvertToRaw(struct cell source,struct raw_cell *dest)
{
	dest->data[0] = source.id % 256; //first byte
	dest->data[1] = source.id >> 8; //last byte
	dest->data[2] = source.energy % 256;
	dest->data[3] = source.energy >> 8;
	dest->data[4] = source.fat;
	dest->data[5] = source.gen_select;
	
	dest->data[6] = source.stick_up;
	dest->data[7] = source.stick_down;
	dest->data[8] = source.stick_left;
	dest->data[9] = source.stick_right;
	
	dest->data[10] = source.wait_move;
	
	for(int i=0;i<genome_len;i++)
	{
		dest->data[10+i] = source.genome[i].gen;
	}
}

void Save()
{
	FILE * f = fopen("savefile.bin","wb");
	struct raw_cell raw_buff;
	for(int i=0;i<size_x;i++)
	{
		for(int j=0;j<size_y;j++)
		{
			CellConvertToRaw(plate[i][j],&raw_buff);
			int h = 0;
			for(h=0;h<11;h++)
			{
				putc(raw_buff.data[h],f);
			}
			for(;h<256;h++)
			{
				putc(raw_buff.data[h],f);
			}
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
			setfillstyle(1,COLOR(l+20,l+20,l));
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

void CommandInput(char *str)
{
	if(strcmp(str,"save")==0)
	{
		Save();
		TextShow("Plate Saved! Check file savefile.bin",2000,0,0);
	}
	if(strcmp(str,"light")==0)
	{
		TextShow("Showing LightMap. . .",500,0,0);
		LightShow();
		getch();
	}
}

void GetCommand()
{
	char buf[190] = {0};
	char command[128] = {0};
	char c;
	int i = 0;
	if(kbhit()==1)
	{
		if(getch()=='c')
		{
			TextShow("Print Command",5,0,0);
			while(true)
			{
				c = getch();
				if(c==';') break;
				command[i] = c;
				TextShow(command,5,0,8);
				i++;
				if(i>=128)
				{
					TextShow("Too long command!",2000,0,0);
					break;
				}
			}
			sprintf(buf,"Command:%s",command);
			TextShow(buf,1000,0,16);
			CommandInput(command);
		}
	}
}

int main()
{
	srand(time(NULL));
	initwindow(size_x*cellsize,size_y*cellsize,"CyberBiology v0.1");
	outtextxy(10,10,"Loading...");
	//initialization of cell plate

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
	
	//init end
	
	//spawn random things
	
	for(int i=0;i<size_x;i++)
	{
		srand(rand()+rand());
		for(int j=0;j<size_y;j++)
		{
			CellSpawn(i,j,3);
		}
	}
	
	LightGen();
	//spawning end
	int dev_time = 0;
	swapbuffers();
	DrawCells();
	swapbuffers();
	while(dev_time<10000)
	{
		GetCommand();
		if(dev_time%20==0)
		{
			DrawCells();
			printf("%d\n",dev_time);
			swapbuffers();
		}
		WorldTick();
		dev_time++;
	}
	getch();
	return 0;
}