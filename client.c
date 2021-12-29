#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT 8100

void view_booking(int sock);
int menu(int sock, int type);
int do_admin_action(int sock, int action);
int do_action(int sock, int opt);

//---------------------------------------------------------------
void view_booking(int sock){
	int entries;
	read(sock, &entries, sizeof(int));
	if(entries == 0) printf("No records found.\n");
	else printf("Your recent bookings are :\n");
	while(!getchar());
	while(entries--){

		int bid, bks_seat, bke_seat, cancelled;
		char trainname[20];
		read(sock,&bid, sizeof(bid));
		read(sock,&trainname, sizeof(trainname));
		read(sock,&bks_seat, sizeof(int));
		read(sock,&bke_seat, sizeof(int));
		read(sock,&cancelled, sizeof(int));
		if(!cancelled)
		printf("BookingID: %d\t1st Ticket: %d\tLast Ticket: %d\tTRAIN :%s\n", bid, bks_seat, bke_seat, trainname);
	}
	printf("Press any key to continue...\n");
	while(getchar()!='\n');
	getchar();
}
//---------------------------------------------------------------

int do_action(int sock, int opt)
{
	switch(opt)
	{
		case 1:
		{
			//book tickets------------------------------------------------
			int trains, trainid, trainavseats, trainno, required_seats;
			char trainname[20];
			write(sock, &opt, sizeof(opt));
			read(sock, &trains, sizeof(trains));
			printf("ID\tT_NO\tAV_SEAT\tTRAIN NAME\n");
			while(trains--){
				read(sock, &trainid, sizeof(trainid));
				read(sock, &trainno, sizeof(trainno));
				read(sock, &trainavseats, sizeof(trainavseats));
				read(sock, &trainname, sizeof(trainname));
				if(strcmp(trainname, "deleted")!=0)//for deleting a train i am using deleted
				printf("%d\t%d\t%d\t%s\n", trainid, trainno, trainavseats, trainname);
			}
			printf("Enter the train ID: ");
			scanf("%d", &trainid);
			write(sock, &trainid, sizeof(trainid));
			read(sock, &trainavseats, sizeof(trainavseats));
			printf("Enter the number of seats: ");
			scanf("%d", &required_seats);
			if(trainavseats>=required_seats && required_seats>0)
				write(sock, &required_seats, sizeof(required_seats));
			else{
				required_seats = -1;
				write(sock, &required_seats, sizeof(required_seats));
			}
			read(sock, &opt, sizeof(opt));

			if(opt == 1) printf("Tickets booked successfully\n");
			else printf("Tickets were not booked. Please try again.\n");
			printf("Press any key to continue...\n");
			while(getchar()!='\n');
			getchar();
			while(!getchar());
			return 1;

		//--------------------------------------------------------------
		}
		case 2:{
			//View your bookings----------------------------------------

			write(sock, &opt, sizeof(opt));
			view_booking(sock);
			read(sock, &opt, sizeof(opt));
			return 2;

			//--------------------------------------------------------------
		}
		case 3:
		{
			//-----------------------------------------------------update bookings
			int val;
			write(sock, &opt, sizeof(opt));
			view_booking(sock);
			printf("Enter the booking id to be updated: "); scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Booking ID
			printf("What information do you want to update:\n1. Increase No of Seats\n2. Decrease No of Seats\nYour Choice: ");
			scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Increase or Decrease
			if(val == 1){
				printf("How many tickets do you want to increase: ");scanf("%d",&val);
				write(sock, &val, sizeof(int));	//No of Seats
			}else if(val == 2){
				printf("How many tickets do you want to decrease: ");scanf("%d",&val);
				write(sock, &val, sizeof(int));	//No of Seats
			}
			read(sock, &opt, sizeof(opt));
			if(opt==-7)
			{
				printf("Not authentic user.Operation failed.\n");
			}
			else if(opt == -2)
				printf("Operation failed. No more available seats\n");
			else if(opt==-3)
				printf("Operation failed. Seats to be decreased are more than seats which are booked.\n");
			else printf("Operation succeded.\n");
			while(getchar()!='\n');
			getchar();
			return 3;
		}
		case 4:
		{
			//-------------------------------------------------cancel booking
			write(sock, &opt, sizeof(opt));
			view_booking(sock);
			int val;
			printf("Enter the booking id to be deleted: "); scanf("%d", &val);
			write(sock, &val, sizeof(int));	//Booking ID
			read(sock, &opt, sizeof(opt));
			if(opt==-7)  //checkinng authenticity
			{
				printf("Not authentic user. Operation failed");
			}
			else if(opt == -2)
				printf("Operation failed. No more available seats\n");

			else printf("Operation succeded.\n");
			while(getchar()!='\n');
			getchar();
			return 3;
			}
		case 5:
		{
			//-------------------------------------logout
			write(sock, &opt, sizeof(opt));
			read(sock, &opt, sizeof(opt));
			if(opt == 5) printf("Logged out successfully.\n");
			while(getchar()!='\n');
			getchar();
			return -1;
			break;
		}
		default: return -1;
	}
}


//--------------------------------------------------------------
//-------------------------------------------------------------menu
int menu(int sock, int type)
{
	int opt = 0;
	if(type == 1 || type == 2){
		system("clear");
		printf("=======================================================\n");
		printf("1. Book Ticket\n");
		printf("2. View Bookings\n");
		printf("3. Update Booking\n");
		printf("4. Cancel booking\n");
		printf("5. Logout\n");
		printf("=======================================================\n");
		printf("Enter Your Choice: ");
		scanf("%d", &opt);
		return do_action(sock, opt);
	}
	else{
		system("clear");
		printf("=======================================================\n");
		printf("1. Add Train\n");
		printf("2. Delete Train\n");
		printf("3. Modify Train\n");
		printf("4. Add Root User\n");
		printf("5. Delete User\n");
		printf("6. Logout\n");
		printf("=======================================================\n");
		printf("Enter Your Choice: ");
		scanf("%d", &opt);
		return do_admin_action(sock, opt);
	}
}
//----------------------------------------------------------------
int do_admin_action(int sock, int opt){
	switch(opt)
	{
		case 1:
		{
			int tno;
			char tname[20];
			write(sock, &opt, sizeof(opt));
			printf("Enter Train Name: ");
			scanf("%s", tname);
			printf("Enter Train No. : ");
			scanf("%d", &tno);
			write(sock, &tname, sizeof(tname));
			write(sock, &tno, sizeof(tno));
			read(sock, &opt, sizeof(opt));
			if(opt == 1 )
		    printf("Train Added Successfully.\n");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 2:
		{
			int no_of_trains;
			write(sock, &opt, sizeof(opt));
			read(sock, &no_of_trains, sizeof(int));
			while(no_of_trains>0){
				int tid, tno;
				char tname[20];
				read(sock, &tid, sizeof(tid));
				read(sock, &tname, sizeof(tname));
				read(sock, &tno, sizeof(tno));
				if(strcmp(tname, "deleted")!=0)
					printf("%d.\t%d\t%s\n", tid, tno, tname);
				no_of_trains--;
			}
			printf("\nEnter the train ID to delete: ");
		    scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			read(sock, &opt, sizeof(opt));
			if(opt != -5) printf("Train deleted successfully\n");
			else printf("Operation cancelled!");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 3:{
			int no_of_trains;
			write(sock, &opt, sizeof(opt));
			read(sock, &no_of_trains, sizeof(int));
			while(no_of_trains>0){
				int tid, tno;
				char tname[20];
				read(sock, &tid, sizeof(tid));
				read(sock, &tname, sizeof(tname));
				read(sock, &tno, sizeof(tno));
				if(!strcmp(tname, "deleted"));else
				printf("%d.\t%d\t%s\n", tid+1, tno, tname);
				no_of_trains--;
			}
			printf("Enter 0 to cancel.\nEnter the train ID to modify: "); scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			printf("What parameter do you want to modify?\n1. Train Name\n2. Train No.\n3. Available Seats\n");
			printf("Your Choice: ");scanf("%d", &no_of_trains);
			write(sock, &no_of_trains, sizeof(int));
			if(no_of_trains == 2 || no_of_trains == 3){
				read(sock, &no_of_trains, sizeof(int));
				printf("Current Value: %d\n", no_of_trains);
				printf("Enter Value: ");scanf("%d", &no_of_trains);
				write(sock, &no_of_trains, sizeof(int));
			}
			else{
				char name[20];
				read(sock, &name, sizeof(name));
				printf("Current Value: %s\n", name);
				printf("Enter Value: ");scanf("%s", name);
				write(sock, &name, sizeof(name));
			}
			read(sock, &opt, sizeof(opt));
			if(opt == 3) printf("Train Data Modified Successfully\n");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 4:{
			write(sock, &opt, sizeof(opt));
			char pass[20],name[10];
			printf("Enetr the name: ");scanf("%s", name);
			strcpy(pass, getpass("Enter a password for the ADMIN account: "));
			write(sock, &name, sizeof(name));
			write(sock, &pass, sizeof(pass));
			read(sock, &opt, sizeof(opt));
			printf("The Account Number for this ADMIN is: %d\n", opt);
			read(sock, &opt, sizeof(opt));
			if(opt == 4)printf("Successfully created ADMIN account\n");
			while(getchar()!='\n');
			getchar();
			return opt;
			break;
		}
		case 5: {
			int choice, users, id;
			write(sock, &opt, sizeof(opt));
			printf("What kind of account do you want to delete?\n");
			printf("1. Customer\n2. Agent\n3. Admin\n");
			printf("Your Choice: ");
			scanf("%d", &choice);
			write(sock, &choice, sizeof(choice));
			read(sock, &users, sizeof(users));
			while(users--){
				char name[10];
				read(sock, &id, sizeof(id));
				read(sock, &name, sizeof(name));
				if(strcmp(name, "deleted")!=0)
				printf("%d\t%s\n", id, name);
			}
			printf("Enter the ID to delete: ");scanf("%d", &id);
			write(sock, &id, sizeof(id));
			read(sock, &opt, sizeof(opt));
			if(opt == 5) printf("Successfully deleted user\n");
			while(getchar()!='\n');
			getchar();
			return opt;
		}
		case 6: {write(sock, &opt, sizeof(opt));
			read(sock, &opt, sizeof(opt));
			if(opt==6) printf("Logged out successfully.\n");
			while(getchar()!='\n');
			getchar();
			return -1;
			break;}
		default: return -1;
	}
}

//-----------------------------------------------------------------
int main(int argc, char * argv[]){

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(client_fd == -1){
		printf("Socket Creation Failed\n");
		exit(0);
	}
	struct sockaddr_in sa;
	sa.sin_family=AF_INET;
	sa.sin_port= htons(PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if(connect(client_fd, (struct sockaddr *)&sa, sizeof(sa))==-1){
		printf	("Connection Failed\n");
		exit(0);
	}
	printf("Connection Established\n");

while(1)
{
	int choice;
	system("clear");
	printf("*****************************\n");
	printf("* WELCOME TO ONLINE RAILWAY TICKET BOOKING SYSTEM *\n");
	printf("*****************************\n");
	printf("1. Sign In\n");
	printf("2. Sign Up\n");
	printf("3. Exit\n");
	printf("*****************************\n");
	printf("Enter Your Choice\n");
	scanf("%d", &choice);
	write(client_fd, &choice, sizeof(choice));
	if(choice==1)
	{
		//------------------------------------login in-------------------
		int type, acc_no;
		char password[20];
		printf("Enter the type of account:\n");
		printf("1. Customer\n2. Agent\n3. Admin\n");
		printf("Your Response: ");
		scanf("%d", &type);
		printf("Enter Your Account Number: ");
		scanf("%d", &acc_no);
		strcpy(password,getpass("Enter the password: "));

		write(client_fd, &type, sizeof(type));
		write(client_fd, &acc_no, sizeof(acc_no));
		write(client_fd, &password, strlen(password));

		int valid_login;
		read(client_fd, &valid_login, sizeof(valid_login));
		if(valid_login == 1){
			while(menu(client_fd, type)!=-1);
			system("clear");

		}
		else
		{
			printf("Multiple login not allowed.\n");
			while(getchar()!='\n');
			getchar();

		}
	//------------------------------------------------------
	}
	else if(choice==2)
	{
		//---------------------------------------------------create accounts----------------------
		int type, acc_no;
		char password[20],  name[10];
		printf("Enter the type of account:\n");
		printf("1. Customer\n2. Agent\n3. Admin\n");
		printf("Enter your choice: ");
		scanf("%d", &type);
		printf("Enter your name: ");
		scanf("%s", name);
		strcpy(password,getpass("Enter the password: "));

		write(client_fd, &type, sizeof(type));
		write(client_fd, &name, sizeof(name));
		write(client_fd, &password, strlen(password));

		read(client_fd, &acc_no, sizeof(acc_no));
		printf("Remember the account no of further login: %d\n", acc_no);
		while(getchar()!='\n');
		getchar();
	//-----------------------------------------------------------------------------
	}
	else
	{
		//exiting
		exit(0);
	}

 }
	close(client_fd);

	return 0;
}
