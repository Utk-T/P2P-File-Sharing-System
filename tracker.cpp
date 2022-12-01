#include <bits/stdc++.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <thread>
using namespace std;

struct user
{
	string serving_ip;
	string serving_port;
	string username;
	string password;
	bool loggedin;
	unordered_map<string,string> fname_path;
};

struct group
{
	string groupID;
	string owner;
	vector<string> members;
	vector<string> group_files;
	vector<string> request_pending;
	
	bool check_member(string s)
	{
		int i=0;
		while(i<members.size())
		{
			if(s==members[i])
			{
				return 1;
			}
			i++;
		}
		return 0;
	}
	
	bool check_file(string s)
	{
		int i=0;
		while(i<group_files.size())
		{
			if(s==group_files[i])
			{
				return 1;
			}
			i++;
		}
		return 0;
	}
	
	bool check_request(string s)
	{
		int i=0;
		while(i<request_pending.size())
		{
			if(s==request_pending[i])
			{
				return 1;
			}
			i++;
		}
		return 0;
	}
};

struct file
{
	string filename;
	long long f_size;
	string sha1;
	vector<pair<string,bool>> users_files_sharing;
	
	bool check_user(string username)
	{
		int i=0;
		while(i<users_files_sharing.size())
		{
			if(users_files_sharing[i].first==username)
			{
				return 1;
			}
			i++;
		}
		return 0;
	}
};

unordered_map<string,user*> users;	
unordered_map<string,file*> files;	
unordered_map<string,group*> groups;	

void connect_with_client(int newsockfd)
{
	string s;
	cout<<"Serving the client having file descriptor "<<newsockfd<<"."<<endl;
	while(1)
	{
		char buffer[10000];
		int temp_r=read(newsockfd,buffer,10000);
		if(temp_r==0)
		{
			cout<<"Received nothing. Shutting down client "<<newsockfd<<endl;
			return;
		}
		cout<<"Client "<<newsockfd<<": "<<buffer<<"\n";
		vector<string> cmds;
		char *token=strtok(buffer," ");
		while(token!=NULL)
		{
			cmds.push_back(token);
			token=strtok(NULL," ");
		}
		for(int i=0;i<10000;i++)
		{
			buffer[i]='\0';
		}
		
		if(cmds[0]=="create_user")
		{
			if(users.find(cmds[1])!=users.end())
			{
				s="User exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else{
				user* u=new user();
				u->username=cmds[1];
				u->password=cmds[2];
				u->loggedin=0;
				users[cmds[1]]=u;
				s="User with username "+cmds[1]+" created successfully.";
				write(newsockfd,s.c_str(),s.size());
			}
		}
		
		else if(cmds[0] == "logout")
		{
			if(users.find(cmds[1])==users.end())
			{
				s="No user exists with username as "+cmds[1];
				write(newsockfd,s.c_str(),s.size());
			}
			else
			{
				users[cmds[1]]->serving_ip="";
				users[cmds[1]]->serving_port="";
				users[cmds[1]]->loggedin=0;
				s="Logout successful.";
				write(newsockfd,s.c_str(),s.size());
			}
		}
		
		else if(cmds[0] == "login")
		{
			if(users.find(cmds[1])==users.end())
			{
				s="No user exists with username as "+cmds[1];
				write(newsockfd,s.c_str(),s.size());
			}
			else
			{
				if(users[cmds[1]]->password!=cmds[2])
				{
					s="Incorrect password.";
					write(newsockfd,s.c_str(),s.size());
				}
				else
				{
					users[cmds[1]]->loggedin=1;
					users[cmds[1]]->serving_ip=cmds[3];
					users[cmds[1]]->serving_port=cmds[4];
					s="User logged in.";
					write(newsockfd,s.c_str(),s.size());
				}
			}
		}

		else if(cmds[0] == "create_group")
		{
			if(users.find(cmds[2])==users.end())
			{
				s="No user exists with username as "+cmds[2];
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups.find(cmds[1])!=groups.end())
			{
				s="Group exists.\n";
				write(newsockfd,s.c_str(),s.size());
			}
			else
			{
				group* g=new group();
				g->groupID=cmds[1];
				g->owner=cmds[2];
				g->members.push_back(cmds[2]);
				groups[cmds[1]]=g;				
				s="Group "+cmds[1]+" created successfully!";
				write(newsockfd,s.c_str(),s.size());				
			}
		}
		
		else if(cmds[0] == "join_group")
		{
			if(users.find(cmds[2])==users.end())
			{
				s="No user exists with username as "+cmds[2];
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups.find(cmds[1])==groups.end())
			{
				s="No such group exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups[cmds[1]]->check_member(cmds[2]))
			{
				s="You are already a member of this group.";
				write(newsockfd,s.c_str(),s.size());				
			}
			else
			{
				groups[cmds[1]]->request_pending.push_back(cmds[2]);
				s="Request to join group "+cmds[1]+" is sent.";
				write(newsockfd,s.c_str(),s.size());			
			}
		}
		
		else if(cmds[0] == "leave_group")
		{
			if(users.find(cmds[2])==users.end())
			{
				s="No user exists with username as "+cmds[2];
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups.find(cmds[1])==groups.end())
			{
				s="No such group exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups[cmds[1]]->check_member(cmds[2])==0)
			{
				s="You are not a member of this group";
				write(newsockfd,s.c_str(),s.size());				
			}
			else if(groups[cmds[1]]->owner==cmds[2])
			{
				groups.erase(cmds[1]);
				s="Owner left the group. Group deleted.";
				write(newsockfd,s.c_str(),s.size());
			}
			else
			{
				groups[cmds[1]]->members.erase(find(groups[cmds[1]]->members.begin(),groups[cmds[1]]->members.end(),cmds[2]));
				s="You left the group "+cmds[1]+".";
				write(newsockfd,s.c_str(),s.size());			
			}
		}
	
		else if(cmds[0] == "list_requests")
		{
			if(users.find(cmds[2])==users.end())
			{
				s="No user exists with username as "+cmds[2];
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups.find(cmds[1])==groups.end())
			{
				s="No such group exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups[cmds[1]]->owner!=cmds[2])
			{
				s="You are not the owner of this group.";
				write(newsockfd,s.c_str(),s.size());			
			}
			else{
				s="";
				int i=0;
				while(i<groups[cmds[1]]->request_pending.size())
				{
					s=s+groups[cmds[1]]->request_pending[i]+"\n";
					i++;
				}
				if(groups[cmds[1]]->request_pending.size()==0)
				{
					s="No requests pending.\n";
				}
				write(newsockfd,s.c_str(),s.size());				
			}
		}
		
		else if(cmds[0] == "accept_request")
		{
			if(users.find(cmds[3])==users.end())
			{
				s="No user exists with username as "+cmds[3];
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups.find(cmds[1])==groups.end())
			{
				s="No such group exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups[cmds[1]]->owner!=cmds[3])
			{
				s="You are not the owner of this group.";
				write(newsockfd,s.c_str(),s.size());				
			}
			else if(!groups[cmds[1]]->check_request(cmds[2]))
			{
				s="No pending request found for this user.";
				write(newsockfd,s.c_str(),s.size());			
			}
			else
			{
				groups[cmds[1]]->request_pending.erase(find(groups[cmds[1]]->request_pending.begin(),groups[cmds[1]]->request_pending.end(),cmds[2]));
				groups[cmds[1]]->members.push_back(cmds[2]);
				s="Request accepted.";
				write(newsockfd,s.c_str(),s.size());
			}
		}
		
		else if(cmds[0] == "list_groups")
		{
			s="";
			for(auto itr:groups)
			{
				s=s+itr.first+"\n";
			}
			if(s.size()==0)
			{
				s="No Groups found!\n";
			}
			write(newsockfd,s.c_str(),s.size());
		}
		
		else if(cmds[0] == "list_files")
		{
			if(groups.find(cmds[1])==groups.end())
			{
				s="No such group exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else if(!groups[cmds[1]]->check_member(cmds[2]))
			{
				s="You are not a member of this group\n";
				write(newsockfd,s.c_str(),s.size());				
			}
			else
			{
				s="";
				for(auto st: groups[cmds[1]]->group_files)
				{
					for(auto itr:files[st]->users_files_sharing)
					{
						if(itr.second==1)
						{
							s=s+st+"\n";
						}
					}
				}
				if(s=="")
				{
					s="No files in this group\n";
				}
				write(newsockfd, s.c_str() ,s.size());				
			}
		}

		else if(cmds[0] == "upload_file")
		{
			if(users.find(cmds[2])==users.end())
			{
				s="No user exists with username as "+cmds[2];
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups.find(cmds[1])==groups.end())
			{
				s="No such group exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else
			{
				if(!groups[cmds[1]]->check_file(cmds[3]))
				{
					groups[cmds[1]]->group_files.push_back(cmds[3]);
				}
				if(files.find(cmds[3])!=files.end())
				{
					if(files[cmds[3]]->check_user(cmds[2]))
					{
						string msg="File already present.";
						write(newsockfd, s.c_str(),s.size());
					}
					else
					{
						files[cmds[3]]->users_files_sharing.push_back(make_pair(cmds[2],1));
						users[cmds[2]]->fname_path[cmds[3]]=cmds[4];
						string s=cmds[3]+" uploaded successfully";
						write(newsockfd,s.c_str(),s.size());						
					}		
					
				}
				else
				{
					file* temp=new file();
					temp->filename=cmds[3];
					temp->f_size=stoi(cmds[5]);
					temp->users_files_sharing.push_back(make_pair(cmds[2],1));
					files[cmds[3]]=temp;
					users[cmds[2]]->fname_path[cmds[3]] = cmds[4];
					temp->sha1=cmds[6];
					string s=cmds[3]+" uploaded successfully";
					write(newsockfd,s.c_str(),s.size());
				}
			}
		}
		
		else if(cmds[0]=="download_file")
		{
			s="";
			string filetosearch = cmds[1];
			string filepath;
			string grp = cmds[2];
			if(!groups[cmds[2]]->check_member(cmds[3]))
			{
				s="#";
				write(newsockfd,s.c_str(),s.size());
				continue;				
			}
			if(!groups[grp]->check_file(filetosearch))
			{
				s="^";
				write(newsockfd,s.c_str(),s.size());
				continue;
			}
			
			int flag=0;
			for(auto t_user: files[filetosearch]->users_files_sharing)
			{
				if(users[t_user.first]->loggedin && t_user.second)
				{
					flag=1;
					s += users[t_user.first]->serving_ip+" "+users[t_user.first]->serving_port+" ";
					filepath=users[t_user.first]->fname_path[cmds[1]];
				}
			}
			if(flag)
			{
				s=s+filepath;
				write(newsockfd,s.c_str(),s.size());
			}
			else
			{
				s="~ No peers found ~";
				write(newsockfd,s.c_str(),s.size());
			}
		}
		
		else if(cmds[0]=="stop_share")
		{
			if(files.find(cmds[2])==files.end())
			{
				s="No such file exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else if(groups.find(cmds[1])==groups.end())
			{
				s="No such group exists.";
				write(newsockfd,s.c_str(),s.size());
			}
			else if(!groups[cmds[1]]->check_member(cmds[3]))
			{
				s="You are not a member of this group";
				write(newsockfd,s.c_str(),s.size());			
			}
			else
			{
				for(auto i :files[cmds[2]]->users_files_sharing)
				{
					if(i.first==cmds[3])
					{
						i.second=false;
						break;
					}
				}
				s="Stopped sharing";
				write(newsockfd,s.c_str(),s.size());			
			}
		}
		
		else if(cmds[0]=="test")
		{
			for(auto i: users)
			{
				cout<<"Username: "<<i.first<<", Password: "<<i.second->password<<"\n";
			}
			for(auto i: files)
			{
				cout<<"Filename = "<<i.first<<" "<<i.second->f_size<<"\n";
				cout<<"Users with this file:"<<endl;
				for(auto s:i.second->users_files_sharing)
				{
					cout<<s.first<<" , full path = "<<users[s.first]->fname_path[i.first]<<endl;
				}
			}
			s="Done.";
			write(newsockfd,s.c_str(),s.size());	
		}
	}
}

void quit()
{
	string s;
	cin>>s;
	if(s=="quit")
	{
		cout<<"Closing the tracker....."<<endl;
		exit(0);
	}
}

int main(int argc, char const *argv[]) 
{ 
	struct sockaddr_in address; 
	int server_fd, new_socket,opt=1; 

	if(argc != 3)
	{
		cout<<"Invalid arguments\n";
	}
	string tracker_port=argv[2];
	string tracker_ip=argv[1];
	
	cout<<endl<<"Serving to "<<tracker_ip<<":"<<tracker_port<<"\n";
	
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{ 
		cout<<"Error: Socket creation failed.\n"; 
		exit(1); 
	} 
	
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
	{ 
		cout<<"Failed at setsockopt.\n";
		exit(1);
	}
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons(stoi(tracker_port)); 
	
	if(bind(server_fd,(struct sockaddr *)&address,sizeof(address))<0)
	{ 
		cout<<"Error: Binding failed.\n"; 
		exit(1); 
	}
	
	if(listen(server_fd,10)<0)
	{ 
		cout<<"Error: Failed to listen.\n";
		exit(1);
	}
	
	thread exitnow(quit);
	
	int addrlen=sizeof(address);
	while(1)
	{
		if((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0)
		{
			cout<<"Error: Failed to accept.\n";
			exit(1); 
		}
		cout<<"New Socket created."<<endl;
		thread th(connect_with_client,new_socket);
		th.detach();
	}
	return 0; 
} 
