#include"testprimer.h"

DWORD WINAPI nit3run(){

	wait(sem23);	//ceka nit2

	char decFPath[]="1:\\fajl1.dat";
	decFPath[0]=p1;
	FS::declare(decFPath,1);
	wait(mutex); cout<<"Nit3: Najavljeno koriscenje fajla '"<<decFPath<<"'"<<endl; signal(mutex);
	
	//decFPath="2:\\fajl2.dat";
	decFPath[0]=p2;decFPath[7]='2';
	FS::declare(decFPath,1);
	wait(mutex); cout<<"Nit3: Najavljeno koriscenje fajla '"<<decFPath<<"'"<<endl; signal(mutex);
	
	//decFPath="1:\\fajl3.dat";
	decFPath[0]=p1; decFPath[7]='3';
	FS::declare(decFPath,1);
	wait(mutex); cout<<"Nit3: Najavljeno koriscenje fajla '"<<decFPath<<"'"<<endl; signal(mutex);
	
	{
		char filepath1[]="1:\\fajl1.dat";
		filepath1[0]=p1;
		File *f1=FS::open(filepath1);
		wait(mutex); cout<<"Nit3: Otvoren fajl fajl1.dat"<<endl;
		cout<<"Nit3: signal 2"<<endl; signal(mutex);
		signal(sem32);//6.signal
		char filepath2[]="1:\\fajl3.dat";
		filepath2[0]=p1;
		File *f2=FS::open(filepath2);
		wait(mutex); cout<<"Nit3: Kreiran fajl fajl3.dat"<<endl; signal(mutex);
		wait(mutex); cout<<"Nit3: signal 1"<<endl; signal(mutex);
		signal(sem31);//7.signal
		char c;
		while(!f1->eof()){
			f1->read(1,&c);
			f2->write(1,&c);
		}
		wait(mutex); cout<<"Nit3: Sadrzaj fajla 'fajl1.dat' prepisan u 'fajl3.dat'"<<endl; signal(mutex);

		f2->seek(0);
		char *toWr=new char[ulazSize+1];
		f2->read(0x7FFFFFFF,toWr);//namerno veci broj od velicine fajla
		toWr[ulazSize]=0;//zatvaramo string pre ispisa
		wait(mutex); 
			cout<<"Nit3: Sadrzaj fajla 'fajl3.dat'"<<endl;
			cout<<toWr<<endl;
		signal(mutex);
		delete [] toWr;

		delete f1;
		wait(mutex); cout<<"Nit3: Zatvoren fajl fajl1.dat"<<endl; signal(mutex);
		delete f2;
		wait(mutex); cout<<"Nit3: Zatvoren fajl fajl3.dat"<<endl; signal(mutex);
	}
	wait(mutex); cout<<"Nit3: wait 1"<<endl; signal(mutex);
	wait(sem13);//ceka nit 1

	//4.blok
	{
		char filepath[]="2:\\fajl2.dat";
		filepath[0]=p2;
		File *f=FS::open(filepath);
		wait(mutex); cout<<"Nit3: Otvoren fajl fajl2.dat"<<endl; signal(mutex);
		delete f;
		wait(mutex); cout<<"Nit3: Zatvoren fajl fajl2.dat"<<endl; signal(mutex);
	}
	wait(mutex); cout<<"Nit3: wait 1"<<endl; signal(mutex);
	wait(sem13);
	wait(mutex); cout<<"Nit3: wait 2"<<endl; signal(mutex);
	wait(sem23);

	//5.blok
	{
		char filepath[]="2:\\fajl2.dat";
		filepath[0]=p2;
		File *f=FS::open(filepath);
		wait(mutex); cout<<"Nit3: Otvoren fajl fajl2.dat"<<endl; signal(mutex);
		ofstream fout("izlaz1.dat", ios::out|ios::binary);
		char *buff=new char[f->getFileSize()];
		f->read(f->getFileSize(),buff);
		fout.write(buff,f->getFileSize());
		wait(mutex); cout<<"Nit3: Upisan 'fajl2.dat' u fajl os domacina 'izlaz1.dat'"<<endl; signal(mutex);
		delete [] buff;
		fout.close();
		delete f;
		wait(mutex); cout<<"Nit3: Zatvoren fajl fajl2.dat"<<endl; signal(mutex);
		
		//decFPath="1:\\fajl1.dat";
		decFPath[0]=p1; decFPath[7]='1';
		FS::declare(decFPath,0);
		wait(mutex); cout<<"Nit3: Odjavljeno koriscenje fajla '"<<decFPath<<"'"<<endl; signal(mutex);
		
		//decFPath="2:\\fajl2.dat";
		decFPath[0]=p2; decFPath[7]='2';
		FS::declare(decFPath,0);
		wait(mutex); cout<<"Nit3: Odjavljeno koriscenje fajla '"<<decFPath<<"'"<<endl; signal(mutex);
		
		//decFPath="1:\\fajl3.dat";
		decFPath[0]=p1; decFPath[7]='3';
		FS::declare(decFPath,0);
		wait(mutex); cout<<"Nit3: Odjavljeno koriscenje fajla '"<<decFPath<<"'"<<endl; signal(mutex);
	
		FS::unmount(p2);
		wait(mutex); cout<<"Nit3: Demontirana particija p2"<<endl; signal(mutex);
	}

	wait(mutex); cout<<"Nit3: Zavrsena!"<<endl; signal(mutex);
	signal(semMain);
	return 0;
}