#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <signal.h>

#define PORT 8100

struct account{
	int id;
	char name[10];
	char pass[20];
};

struct train{
	int tid;
	char train_name[20];
	int train_no;
	int av_seats;
	int last_seatno_used;
};

struct bookings{
	int bid;
	int type;
	int acc_no;
	int tr_id;
	char trainname[20];
	int seat_start;
	int seat_end;
	int cancelled;
};

char *ACC[3] = {"./database/accounts/customer", "./database/accounts/agent", "./database/accounts/admin"};

void service_request(int sock_fd);
int login(int sock);
int create_account(int sock_fd);
int menu_admin(int sock, int id);
int menu_user(int sock, int id, int type);
void view_booking(int sock, int id, int type);
void view_booking2(int sock, int id, int type, int fd);
void sigstop_sigkill_handler(int sig);
int booking_ticket(int sock,int id,int type);
void sigstop_sigkill_handler(int sig) {
	char str[5];
	printf("Do you want to stop the server(y/n)?\n");
	scanf("%s", str);
	if(strcmp("y", str) == 0) {
		exit(0);
	}
	return;
}


//---------------------------------------------------------------
int login(int sock_fd)
{
	int type, acc_no, fd, valid=1, invalid=0, login_success=0;
	char password[20];
	struct account temp;
	read(sock_fd, &type, sizeof(type));
	read(sock_fd, &acc_no, sizeof(acc_no));
	read(sock_fd, &password, sizeof(password));
	if(type==1)
	{
		if((fd = open("./database/accounts/customer", O_RDWR))==-1)
			printf("File Error\n");
	}
	else if(type==2)
	{
		if((fd = open("./database/accounts/agent", O_RDWR))==-1)
			printf("File Error\n");
	}
	else if(type==3)
	{
		if((fd = open("./database/accounts/admin", O_RDWR))==-1)
			printf("File Error\n");
	}
	struct flock lock;

	lock.l_start = (acc_no-1)*sizeof(struct account);
	lock.l_len = sizeof(struct account);
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();

	if(type == 1){
	// --------------------------------------------------User
		lock.l_type = F_WRLCK;
		fcntl(fd,F_SETLK, &lock);
		lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_CUR);
		read(fd, &temp, sizeof(struct account));
		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock_fd, &valid, sizeof(valid));
				while(menu_user(sock_fd, temp.id, type)!=-1);
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success)
		return 3;
	//-------------------------------------------------------------------
	}
	else if(type == 2){
	//--------------------------------------------------- Agent

		lock.l_type = F_RDLCK;
		fcntl(fd,F_SETLK, &lock);
		lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_SET);
		read(fd, &temp, sizeof(struct account));
		if(temp.id == acc_no){
			if(!strcmp(temp.pass, password)){
				write(sock_fd, &valid, sizeof(valid));
				while(menu_user(sock_fd, temp.id, type)!=-1);
				login_success = 1;
			}
		}
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		if(login_success) return 3;

	}
	else if(type == 3){
		// -----------------------------------------------------Admin
		lock.l_type = F_WRLCK;
		int ret = fcntl(fd,F_SETLK, &lock);
		if(ret != -1){

			lseek(fd, (acc_no - 1)*sizeof(struct account), SEEK_SET);///////CUR
			read(fd, &temp, sizeof(struct account));
			if(temp.id == acc_no){
				if(!strcmp(temp.pass, password)){
					write(sock_fd, &valid, sizeof(valid));
					while(-1!=menu_admin(sock_fd, temp.id));
					login_success = 1;
				}
			}
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
		}
		close(fd);
		if(login_success)
		return 3;
	}
	write(sock_fd, &invalid, sizeof(invalid));
	return 3;
}
//------------------------------------------------------------------


int menu_admin(int sock, int id){
	int op_id;
	read(sock, &op_id, sizeof(op_id));
	if(op_id == 1){
		//add a train
		int tid = 0;
		int tno;
		char tname[20];
		read(sock, &tname, sizeof(tname));
		read(sock, &tno, sizeof(tno));
		struct train temp, temp2;

		temp.tid = tid;
		temp.train_no = tno;
		strcpy(temp.train_name, tname);
		temp.av_seats = 15;
		temp.last_seatno_used = 0;

		int fd = open("./database/train", O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		if(fp == 0){
			write(fd, &temp, sizeof(temp));
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
			write(sock, &op_id, sizeof(op_id));
		}
		else{
			lseek(fd, -1 * sizeof(struct train), SEEK_CUR);
			read(fd, &temp2, sizeof(temp2));
			temp.tid = temp2.tid + 1;
			write(fd, &temp, sizeof(temp));
			write(sock, &op_id, sizeof(op_id));
			lock.l_type = F_UNLCK;
			fcntl(fd, F_SETLK, &lock);
			close(fd);
		}
		return op_id;
	}
	if(op_id == 2){
		int fd = open("./database/train", O_RDWR);

		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		printf("no of train:%d\n",no_of_trains);
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		struct train temp;
		while(fp != lseek(fd, 0, SEEK_CUR)){
			//printf("FP :%d  FD :%ld\n",fp,lseek(fd, 0, SEEK_CUR));
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_name, sizeof(temp.train_name));
			write(sock, &temp.train_no, sizeof(int));
		}
		//int train_id=-1;
		read(sock, &no_of_trains, sizeof(int));
		if(no_of_trains != -5)
		{
			struct train temp;

			lseek(fd, (no_of_trains)*sizeof(struct train), SEEK_SET);
			read(fd, &temp, sizeof(struct train));
			printf("%s is deleted\n", temp.train_name);
			strcpy(temp.train_name,"deleted");
			lseek(fd, -1*sizeof(struct train), SEEK_CUR);
			write(fd, &temp, sizeof(struct train));

		}
		write(sock, &no_of_trains, sizeof(int));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
	}
	if(op_id == 3){
		int fd = open("./database/train", O_RDWR);

		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR)){
			struct train temp;
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_name, sizeof(temp.train_name));
			write(sock, &temp.train_no, sizeof(int));
		}
		read(sock, &no_of_trains, sizeof(int));

		struct train temp;
		lseek(fd, 0, SEEK_SET);
		lseek(fd, (no_of_trains-1)*sizeof(struct train), SEEK_CUR);
		read(fd, &temp, sizeof(struct train));

		read(sock, &no_of_trains,sizeof(int));
		if(no_of_trains == 1){
			char name[20];
			write(sock, &temp.train_name, sizeof(temp.train_name));
			read(sock, &name, sizeof(name));
			strcpy(temp.train_name, name);
		}
		else if(no_of_trains == 2){
			write(sock, &temp.train_no, sizeof(temp.train_no));
			read(sock, &temp.train_no, sizeof(temp.train_no));
		}
		else{
			write(sock, &temp.av_seats, sizeof(temp.av_seats));
			read(sock, &temp.av_seats, sizeof(temp.av_seats));
		}

		no_of_trains = 3;
		printf("%s\t%d\t%d\n", temp.train_name, temp.train_no, temp.av_seats);
		lseek(fd, -1*sizeof(struct train), SEEK_CUR);
		write(fd, &temp, sizeof(struct train));
		write(sock, &no_of_trains, sizeof(int));

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		return op_id;
	}
	if(op_id == 4){
		int type=3, fd, acc_no=0;
		char password[20], name[10];
		struct account temp;
		read(sock, &name, sizeof(name));
		read(sock, &password, sizeof(password));

		if((fd = open(ACC[type-1], O_RDWR))==-1)printf("File Error\n");
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd,F_SETLKW, &lock);
		int fp = lseek(fd, 0, SEEK_END);
		fp = lseek(fd, -1 * sizeof(struct account), SEEK_CUR);
		read(fd, &temp, sizeof(temp));
		temp.id++;
		strcpy(temp.name, name);
		strcpy(temp.pass, password);
		write(fd, &temp, sizeof(temp));
		write(sock, &temp.id, sizeof(temp.id));
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);

		close(fd);
		op_id=4;
		write(sock, &op_id, sizeof(op_id));
		return op_id;
	}
	if(op_id == 5){
		int type, id;
		struct account var;
		read(sock, &type, sizeof(type));

		int fd = open(ACC[type - 1], O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		int fp = lseek(fd, 0 , SEEK_END);
		int users = fp/ sizeof(struct account);
		write(sock, &users, sizeof(int));

		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR)){
			read(fd, &var, sizeof(struct account));
			write(sock, &var.id, sizeof(var.id));
			write(sock, &var.name, sizeof(var.name));
		}

		read(sock, &id, sizeof(id));
		if(id == 0){write(sock, &op_id, sizeof(op_id));}
		else{
			lseek(fd, 0, SEEK_SET);
			lseek(fd, (id-1)*sizeof(struct account), SEEK_CUR);
			read(fd, &var, sizeof(struct account));
			lseek(fd, -1*sizeof(struct account), SEEK_CUR);
			strcpy(var.name,"deleted");
			strcpy(var.pass, "");
			write(fd, &var, sizeof(struct account));
			write(sock, &op_id, sizeof(op_id));
		}

		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);

		close(fd);

		return op_id;
	}
	if(op_id == 6) {
		write(sock,&op_id, sizeof(op_id));
		return -1;
	}
}
//-------------------------------------------------------------------
int menu_user(int sock, int id, int type)
{
	int op_id;
	read(sock, &op_id, sizeof(op_id));
	if(op_id == 1)
	{
		//--------------------------------booking a ticket

		int seats=booking_ticket(sock,id,type);
		if(seats<=0)
			op_id = -1;
		write(sock, &op_id, sizeof(op_id));
		return 1;
		//-----------------------------------------------------------------
	}

	if(op_id == 2)
	{
		//------------------------------------------------viewing bookings
		view_booking(sock, id, type);

		write(sock, &op_id, sizeof(op_id));
		return 2;
		//----------------------------------------------------------------------
	}
	if(op_id == 3)
	{
		//-----------------------------------------------------------update booking
		view_booking(sock, id, type);

		int fd1 = open("./database/train", O_RDWR);
		int fd2 = open("./database/bookings", O_RDWR);
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd1, F_SETLKW, &lock);
		fcntl(fd2, F_SETLKW, &lock);

		int val;
		struct train temp1;
		struct bookings temp2;
		read(sock, &val, sizeof(int));	//Read the Booking ID to updated
		// read the booking to be updated
		lseek(fd2, 0, SEEK_SET);
		lseek(fd2, val*sizeof(struct bookings), SEEK_CUR);
		read(fd2, &temp2, sizeof(temp2));
		if(id==temp2.acc_no)
		{
		lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
		printf("%d %s %d\n", temp2.tr_id, temp2.trainname, temp2.seat_end);
		// read the train details of the booking
		lseek(fd1, 0, SEEK_SET);
		lseek(fd1, (temp2.tr_id-1)*sizeof(struct train), SEEK_CUR);
		read(fd1, &temp1, sizeof(temp1));
		lseek(fd1, -1*sizeof(struct train), SEEK_CUR);
		printf("%d %s %d\n", temp1.tid, temp1.train_name, temp1.av_seats);


		read(sock, &val, sizeof(int));	//Increase or Decrease


		if(val==1)
		{//increase---------------------------------------------------
			read(sock, &val, sizeof(int)); //No of Seats
			if(temp1.av_seats>= val){
				temp2.cancelled = 1;
				temp1.av_seats += val;
				write(fd2, &temp2, sizeof(temp2));

				int tot_seats = temp2.seat_end - temp2.seat_start + 1 + val;
				struct bookings bk;

				int fp2 = lseek(fd2, 0, SEEK_END);
				lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
				read(fd2, &bk, sizeof(struct bookings));

				bk.bid++;
				bk.type = temp2.type;
				bk.acc_no = temp2.acc_no;
				bk.tr_id = temp2.tr_id;
				bk.cancelled = 0;
				strcpy(bk.trainname, temp2.trainname);
				bk.seat_start = temp1.last_seatno_used + 1;
				bk.seat_end = temp1.last_seatno_used + tot_seats;

				temp1.av_seats -= tot_seats;
				temp1.last_seatno_used = bk.seat_end;

				write(fd2, &bk, sizeof(bk));
				write(fd1, &temp1, sizeof(temp1));
			}
			else{
				op_id = -2;
				write(sock, &op_id, sizeof(op_id));
			}
		}
		else
		{//decrease	------------------------------------------------
			read(sock, &val, sizeof(int)); //No of Seats
			if(temp2.seat_end - val +1 == temp2.seat_start){
				temp2.cancelled = 1;
				temp1.av_seats += val;
			}
			else if(temp2.seat_end - val +1 < temp2.seat_start)
			{
				op_id=-3;
			}
			else{
				temp2.seat_end -= val;
				temp1.av_seats += val;
			}
			write(fd2, &temp2, sizeof(temp2));
			write(fd1, &temp1, sizeof(temp1));
		}
		lock.l_type = F_UNLCK;
		fcntl(fd1, F_SETLK, &lock);
		fcntl(fd2, F_SETLK, &lock);
		close(fd1);
		close(fd2);
		//if(op_id>0)
		write(sock, &op_id, sizeof(op_id));
	}
	else
	{
		op_id=-7;
		write(sock, &op_id, sizeof(op_id));
	}
		return 3;

	}
	if(op_id == 4)
	 {
		//----------------------------------------------------------cancel booking
		view_booking(sock, id, type);


		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		int fd1 = open("./database/train", O_RDWR);
		int fd2 = open("./database/bookings", O_RDWR);
		fcntl(fd1, F_SETLKW, &lock);


		int val;
		struct train temp1;
		struct bookings temp2;
		read(sock, &val, sizeof(int));	//Read the Booking ID to deleted
		lseek(fd2, val*sizeof(struct bookings), SEEK_SET);


		lock.l_start = val*sizeof(struct bookings);
		lock.l_len = sizeof(struct bookings);
		fcntl(fd2, F_SETLKW, &lock);


		read(fd2, &temp2, sizeof(temp2));

		if(id==temp2.acc_no){                          //checking for authentic user
		lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
		printf("%d %s %d\n", temp2.tr_id, temp2.trainname, temp2.seat_end);
		// read the train details of the booking

		lseek(fd1, (temp2.tr_id)*sizeof(struct train), SEEK_SET); ///// -1 removed
		lock.l_start = (temp2.tr_id)*sizeof(struct train);
		lock.l_len = sizeof(struct train);
		fcntl(fd1, F_SETLKW, &lock);
		read(fd1, &temp1, sizeof(temp1));
		lseek(fd1, -1*sizeof(struct train), SEEK_CUR);
		printf("%d %s %d\n", temp1.tid, temp1.train_name, temp1.av_seats);
		temp1.av_seats += temp2.seat_end - temp2.seat_start + 1;
		temp2.cancelled=1;
		write(fd2, &temp2, sizeof(temp2));
		write(fd1, &temp1, sizeof(temp1));

		lock.l_type = F_UNLCK;
		fcntl(fd1, F_SETLK, &lock);
		fcntl(fd2, F_SETLK, &lock);
		close(fd1);
		close(fd2);
		write(sock, &op_id, sizeof(op_id));
	}
	else  //not authentic user
	{
		op_id=-7;
		write(sock, &op_id, sizeof(op_id));
	}
		return 4;
	}
	if(op_id == 5)
	 {
		write(sock, &op_id, sizeof(op_id));
		return -1;
	}

	return 0;
}


int create_account(int sock_fd)
{
	int type, fd, acc_no=0;
	char password[20], name[10];
	struct account a;

	read(sock_fd, &type, sizeof(type));
	read(sock_fd, &name, sizeof(name));
	read(sock_fd, &password, sizeof(password));

	if(type==1)
	{
		if((fd = open("./database/accounts/customer", O_RDWR))==-1)
			printf("File Error\n");
	}
	else if(type==2)
	{
		if((fd = open("./database/accounts/agent", O_RDWR))==-1)
			printf("File Error\n");
	}
	else if(type==3)
	{
		if((fd = open("./database/accounts/admin", O_RDWR))==-1)
			printf("File Error\n");
	}
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();

	fcntl(fd,F_SETLKW, &lock);

	int pos = lseek(fd, 0, SEEK_END);

	if(pos==0)
	{
	//----------------------------1st time register
		a.id = 1;
		strcpy(a.name, name);
		strcpy(a.pass, password);
		write(fd, &a, sizeof(a));
		write(sock_fd, &a.id, sizeof(a.id));
	}
	else{
		//------------------------------------not 1st register
		pos = lseek(fd, -1 * sizeof(struct account), SEEK_END);
		read(fd, &a, sizeof(a));
		a.id++;
		strcpy(a.name, name);
		strcpy(a.pass, password);
		write(fd, &a, sizeof(a));
		write(sock_fd, &a.id, sizeof(a.id));
	}

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);

	close(fd);
	return 3;
}

//---------------------------------------------------------------booking------------------
int booking_ticket(int sock,int id,int type)
{

	int fd = open("./database/train", O_RDWR);

		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		lock.l_pid = getpid();

		fcntl(fd, F_SETLKW, &lock);

		struct train temp;
		int fp = lseek(fd, 0, SEEK_END);
		int no_of_trains = fp / sizeof(struct train);
		write(sock, &no_of_trains, sizeof(int));
		lseek(fd, 0, SEEK_SET);
		while(fp != lseek(fd, 0, SEEK_CUR))
		{
			read(fd, &temp, sizeof(struct train));
			write(sock, &temp.tid, sizeof(int));
			write(sock, &temp.train_no, sizeof(int));
			write(sock, &temp.av_seats, sizeof(int));
			write(sock, &temp.train_name, sizeof(temp.train_name));
		}
		memset(&temp,0,sizeof(struct train));
		int trainid, seats;
		read(sock, &trainid, sizeof(trainid));
		lseek(fd, trainid*sizeof(struct train), SEEK_SET);
		read(fd, &temp, sizeof(struct train));
		write(sock, &temp.av_seats, sizeof(int));
		read(sock, &seats, sizeof(seats));
		if(seats>0){
			temp.av_seats -= seats;
			int fd2 = open("./database/bookings", O_RDWR);
			fcntl(fd2, F_SETLKW, &lock);
			struct bookings bk;
			int fp2 = lseek(fd2, 0, SEEK_END);
			if(fp2 > 0)
			{
				lseek(fd2, -1*sizeof(struct bookings), SEEK_CUR);
				read(fd2, &bk, sizeof(struct bookings));
				bk.bid++;
			}
			else
			bk.bid = 0;
			bk.type = type;
			bk.acc_no = id;
			bk.tr_id = trainid;
			bk.cancelled = 0;
			strcpy(bk.trainname, temp.train_name);
			bk.seat_start = temp.last_seatno_used + 1;
			bk.seat_end = temp.last_seatno_used + seats;
			temp.last_seatno_used = bk.seat_end;
			write(fd2, &bk, sizeof(bk));
			lock.l_type = F_UNLCK;
			fcntl(fd2, F_SETLK, &lock);
		 	close(fd2);
			lseek(fd, -1*sizeof(struct train), SEEK_CUR);
			write(fd, &temp, sizeof(temp));
		}
		fcntl(fd, F_SETLK, &lock);
	 	close(fd);
	 	return seats;

}
//------------------------------ view booking-----------------------------------------------
void view_booking(int sock, int id, int type){
	int fd = open("./database/bookings", O_RDONLY);
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_whence = SEEK_SET;
	lock.l_pid = getpid();

	fcntl(fd, F_SETLKW, &lock);

	int fp = lseek(fd, 0, SEEK_END);
	int entries = 0;
	if(fp == 0)
		write(sock, &entries, sizeof(entries));
	else{
		struct bookings bk[10];
		while(fp>0 && entries<10){
			struct bookings temp;
			fp = lseek(fd, -1*sizeof(struct bookings), SEEK_CUR);
			read(fd, &temp, sizeof(struct bookings));
			if(temp.acc_no == id && temp.type == type)
				bk[entries++] = temp;
			fp = lseek(fd, -1*sizeof(struct bookings), SEEK_CUR);
		}
		write(sock, &entries, sizeof(entries));
		for(fp=0;fp<entries;fp++){
			write(sock, &bk[fp].bid, sizeof(bk[fp].bid));
			write(sock, &bk[fp].trainname, sizeof(bk[fp].trainname));
			write(sock, &bk[fp].seat_start, sizeof(int));
			write(sock, &bk[fp].seat_end, sizeof(int));
			write(sock, &bk[fp].cancelled, sizeof(int));
		}
	}
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
}

//----------------------------------------------------------------------------
void service_request(int sock_fd){
	int choice;
	read(sock_fd, &choice, sizeof(int));
	printf("Client connected\n");
	while(1)
	{
		if(choice==1)
		{
			login(sock_fd);
			read(sock_fd, &choice, sizeof(int));
		}
		else if(choice==2)
		{
			create_account(sock_fd);
			read(sock_fd, &choice, sizeof(int));
		}
		if(choice==3)
		{
			break;
		}
	}
	close(sock_fd);
	printf("Client disconnected\n");
}




int main(){

	//signal handling-------------------------
	signal(SIGTSTP, sigstop_sigkill_handler);
	signal(SIGINT, sigstop_sigkill_handler);
	signal(SIGQUIT, sigstop_sigkill_handler);


	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1) {
		printf("socket creation failed\n");
		exit(0);
	}
	int optval = 1;
	int optlen = sizeof(optval);
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, optlen)==-1){
		printf("set socket options failed\n");
		exit(0);
	}
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(PORT);

	if(bind(sockfd, (struct sockaddr *)&sa, sizeof(sa))==-1){
		printf("binding port failed\n");
		exit(0);
	}
	if(listen(sockfd, 100)==-1){
		printf("listen failed\n");
		exit(0);
	}
	while(1){
		int connectedfd;
		if((connectedfd = accept(sockfd, (struct sockaddr *)NULL, NULL))==-1){
			printf("connection error\n");
			exit(0);
		}
		if(fork()==0) {
			close(sockfd);
			service_request(connectedfd);
			exit(0);
		} else if(fork() > 0) {
			close(connectedfd);
		}
	}

	close(sockfd);
	return 0;
}
