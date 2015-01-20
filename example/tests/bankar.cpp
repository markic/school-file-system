#include <iostream> 
#include <windows.h> 
#include "FS.h" 
#include "part.h" 
#include "file.h" 

using namespace std; 

#define signal(x) ReleaseSemaphore(x,1,NULL)
#define wait(x) WaitForSingleObject(x,INFINITE)
  
  
bool flag1=false;
bool flag2=false;
bool flag3=false;
int flag4=0; 
int flag5=0;
int flag6=0;

HANDLE mut=CreateSemaphore(NULL,1,32,NULL);

HANDLE Nit1, Nit2,Nit3;
  
void run1(){ 
	char sl='A';
	wait(mut);
	cout<<"Nit1 krenula!\n";
	signal(mut);
	char decFPath[]="1:\\fajl1.dat";
	decFPath[0]=sl;
	char decFPath1[]="1:\\fajl2.dat";
	decFPath1[0]=sl;
	char decFPath2[]="1:\\fajl3.dat";
	decFPath2[0]=sl;
    FS::declare(decFPath,1); 
    FS::declare(decFPath1,1); 
	FS::declare(decFPath2,1); 

	

	wait(mut);
	cout<<"Nit1 deklarisala sva tri  fajla!\n"; 
	signal(mut);

	while(!flag2 && !flag3){}
	flag1=true; 
	


	wait(mut);
	cout<<"Nit1 pokusava da otvori f1\n"; 
	signal(mut);
    File* f1=FS::open(decFPath); 
	
	wait(mut);
    cout<<"Nit1 otvorila f1\n"; 
	signal(mut);
	char b[1000]="Operativni sistemi 2";
	f1->write(1000,b);
	
	

	wait(mut);
	cout<<"Nit1 pokusava da otvori f2\n"; 
	signal(mut);
	
	
	
	File* f2=FS::open(decFPath1);

	
	
	wait(mut);
    cout<<"Nit1 je otvorila f2\n"; 
	signal(mut);
	
	
	wait(mut);
	delete f1;
    delete f2; 
	
	

	
    cout<<"Nit1 je zatvorila f1 i f2\n"; 
	signal(mut);
	
    FS::declare(decFPath,0); 
    FS::declare(decFPath1,0); 
	FS::declare(decFPath2,0); 

	wait(mut);
    cout<<"Nit1 je odjavila koriscenje sva tri fajla\n"; 
	signal(mut);
	
	
	wait(mut);
	cout<<"Nit1 zavrsila!\n";
	signal(mut);

    flag4=1; 
} 
  
void run2(){ 
	char sl='A';
	wait(mut);
	cout<<"Nit2 krenula!"<<endl;
	signal(mut);
	char decFPath[]="1:\\fajl1.dat";
	decFPath[0]=sl;
	char decFPath1[]="1:\\fajl2.dat";
	decFPath1[0]=sl;
	char decFPath2[]="1:\\fajl3.dat";
	decFPath2[0]=sl;
	FS::declare(decFPath,1); 
    FS::declare(decFPath1,1);
	FS::declare(decFPath2,1); 
	
	wait(mut);
    cout<<"Nit2 deklarisala sva tri fajla fajla!\n"; 
    signal(mut);

	flag2=true; 
	while(!flag1 && !flag3){}
	
	wait(mut);
	cout<<"Nit2 pokusava da otvori f2\n"; 
	signal(mut);

    File* f1=FS::open(decFPath1);            //should block here if bankars algorythm works 
	
	wait(mut);
    cout<<"Nit2 je otvorila f2\n"; 
	signal(mut);

   
	wait(mut);
    cout<<"Nit2 pokusava da otvori f1\n"; 
	signal(mut);
	File* f2=FS::open(decFPath);			//will wait thread 1 to finish 

	wait(mut);
    cout<<"Nit2 je otvorila f1\n";
	signal(mut);

    wait(mut);
	delete f1; 
    delete f2;
	cout<<"Nit2 je zatvorila f2 i f1\n"; 
	signal(mut);

    FS::declare(decFPath,0); 
    FS::declare(decFPath1,0); 
	FS::declare(decFPath2,0); 

	wait(mut);
    cout<<"Nit2 je odjavila koriscenje sva 3 fajla\n"; 
	signal(mut);

	
	wait(mut);
	cout<<"Nit2 zavrsila!\n";
	signal(mut);
   flag5=1; 
} 

void run3(){ 
	char sl='A';
	wait(mut);
	cout<<"Nit3 krenula!"<<endl;
	signal(mut);
	char decFPath[]="1:\\fajl1.dat";
	decFPath[0]=sl;
	char decFPath1[]="1:\\fajl2.dat";
	decFPath1[0]=sl;
	char decFPath2[]="1:\\fajl3.dat";
	decFPath2[0]=sl;
	FS::declare(decFPath,1); 
    FS::declare(decFPath1,1);
	FS::declare(decFPath2,1);

	
	wait(mut);
    cout<<"Nit3 deklarisala sva tri fajla!\n"; 
    signal(mut);

	flag3=true;
	while(!flag2 && !flag1 ){}
	
	


    File* f1=FS::open(decFPath2);            //should block here if bankars algorythm works 
	
	wait(mut);
    cout<<"Nit3 je otvorila f3\n"; 
	signal(mut);

   

    wait(mut);

	delete f1; 
	cout<<"Nit3 je zatvorila f3\n"; 
	signal(mut);
	char decFPath21[]="1:\\fajl3.dat";
	decFPath21[0]=sl;
    FS::declare(decFPath,0);
	FS::declare(decFPath1,0);
	FS::declare(decFPath21,0);
   

	wait(mut);
    cout<<"Nit3 je odjavila koriscenje sva tri fajla\n"; 
	signal(mut);

	
	wait(mut);
	cout<<"Nit3 zavrsila!\n";
	signal(mut);
   flag6=1; 
} 
  
DWORD ThreadID;
  
int main(int argc,char* argv){ 
	wait(mut);
	cout<<"Main krenuo!"<<endl;
	signal(mut);
   
       Partition* part=new Partition("i1.ini"); 
       char sl= FS::mount(part); 
	   
	  wait(mut);
	  cout<<"Particija mountovana na "<<sl<<":\\"<<endl;
	  signal(mut);
        FS::format('a'); 
        Nit1=CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) run1,NULL,0,&ThreadID); 
        Nit2=CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) run2,NULL,0,&ThreadID); 
		Nit3=CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) run3,NULL,0,&ThreadID); 
		while(!flag4 || !flag5 || !flag6){
		Sleep(1000);	} 
		
		wait(mut);
		cout<<"Sve 3 niti zavrsile!\n"<<endl;
		signal(mut);
        FS::unmount('A'); 
        wait(mut);
		cout<<"Unmount-ovana particija A:\\\n"; 
		signal(mut);
		delete part;
    
	CloseHandle(Nit1);
	CloseHandle(Nit2);
		
	wait(mut);
	cout<<"Main zavrsio\n"<<endl;
	signal(mut);
	//system("pause");
	
    return 0; 
}