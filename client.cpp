#include <bits/stdc++.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <thread>
#include <openssl/sha.h>
#include <fstream>

using namespace std;

bool loggedin;
string uname="";
unordered_map<string,string> fname_path;
int tracker_sock;
vector<string> completed;
vector<string> downloading;
string grpid;

long long findSize(const char file_name[]) 
{ 
	FILE* fp=fopen(file_name, "r");   
	if(fp==NULL)
	{ 
	    printf("File Not Found!\n"); 
	    return -1; 
	}
	fseek(fp,0L,SEEK_END); 
	long int res=ftell(fp);   
	fclose(fp);   
	return res; 
}

int ChunkWriter(string destfile, long long c_num, char str[], long long len)
{	
	long long cnk=strlen(str);	
	long long fsize=findSize(destfile.c_str());
	long long n=c_num*524288;
	fstream out(destfile.c_str(),fstream::in|fstream::out|fstream::binary);
	out.seekp(n,ios::beg);
	out.write(str,len);
	out.close();
	return 1;
}

long long ChunkReader(char *buff, string filepath, long long c_num)
{
	bzero(buff,sizeof(buff));
	long long fsize=findSize(filepath.c_str());
	int len;
	long long n=c_num*524288;
	if(n>=fsize)
	{
		return -1;
	}
	ifstream in;
	in.open(filepath.c_str(),ios::in|ios::binary);
	in.seekg(n,ios::beg);
	len= n+524288 > fsize ? fsize%524288 : 524288;
	in.read(buff,len);
	in.close();
	return len;
}


void listen_to_peer(int newsockfd)
{
	char buffer[10000];
	int temp_r=read(newsockfd,buffer,10000);
	if(temp_r==0)
	{
		cout<<"No response from peer. Disconnected.\n";
		return;
	}
	cout<<"Received from "<<newsockfd<<": "<<buffer<<endl;
	vector<string> cmds;
	char *token = strtok(const_cast<char*>(buffer), " ");
	while(token != NULL)
	{
		cmds.push_back(token);
		token = strtok(NULL," ");
	}
}

void serve_to_peers(string IP,string port)
{
	int sock_fd,opt = 1;
	struct sockaddr_in addr;
	
	if((sock_fd = socket(AF_INET,SOCK_STREAM,0)) == 0)
	{
		cout<<"Error: Socket creation failed.\n";
		return;
	}
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
	{ 
		cout<<"Failed at setsockopt.\n";
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(stoi(port)); 
	
	if(bind(sock_fd, (struct sockaddr *)&addr,sizeof(addr))<0)
	{ 
		cout<<"Error: Binding failed.\n"; 
		exit(1); 
	}
	if (listen(sock_fd,10)<0)
	{ 
		cout<<"Error: Failed to listen.\n";
		exit(1);
	}
	int addrlen = sizeof(addr);
	int new_socket;
	while(1)
	{
		if((new_socket = accept(sock_fd, (struct sockaddr *)&addr,(socklen_t*)&addrlen))<0)
		{
			cout<<"Error: Failed to accept.\n";
			exit(1); 
		}
		cout<<"Connected with peer.\n";
		thread cli_th(listen_to_peer,new_socket);
		cli_th.detach();
	}

}


void execute_download(string file_name,string source_path,string dest_path,vector<string>reply_tokens,string groupid)
{
	downloading.push_back(groupid+" "+file_name);	//download start
	cout<<endl<<"Downloading from the following peers: "<<endl;
	for(int i=0;i<reply_tokens.size();i=i+2)
	{
		cout<<reply_tokens[i]<<" "<<reply_tokens[i+1]<<endl;
	}
	
	long long fsize=findSize(source_path.c_str());
	fstream out;
	out.open(dest_path.c_str(),ios::out|ios::binary);
	long long szcount=fsize;
	string dummy(fsize,'\0');
	out.write(dummy.c_str(),fsize);
	out.close();
	long long len, numchunks= fsize/524288;
	char *res=new char[524288];
	for(long long i=0;i<numchunks;i++)
	{
		len=ChunkReader(res,source_path,i);
		ChunkWriter(dest_path,i,res,len);
	}
	len=ChunkReader(res,source_path,numchunks);
	ChunkWriter(dest_path,numchunks,res,len);
	
	downloading.erase(find(downloading.begin(),downloading.end(),groupid+" "+file_name)); 
	fname_path[file_name]=dest_path; 	
	completed.push_back(groupid+" "+file_name);
	cout<<"Download completed."<<endl;
}

string getFileName(string path)
{
	int s = path.size()-1;
	while(path[s--]!='/');
	s+=2;
	string ans="";
	while(s<path.size())
	{
		ans += path[s];
		s++;
	}
	return ans;
}

void connect_with_tracker(string client_ip,string client_port)
{
	string cmd="",s;
	getline(cin,cmd);
	vector<string> cmds;
	char *token = strtok(const_cast<char*>(cmd.c_str()), " ");
	while(token != NULL)
	{
		cmds.push_back(token);
		token = strtok(NULL," ");
	}
	char buffer[1000];
	if(cmds[0]=="create_user")
	{
		if(cmds.size()!=3)
		{
			cout<<"Error: Invalid arguments while creating a user."<<endl;
			return;
		}
		s="create_user "+cmds[1]+" "+cmds[2];
		write(tracker_sock,s.c_str(),s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;
	}
	
	else if(cmds[0]=="login")
	{
		if(cmds.size()!=3)
		{
			cout<<"Error: Invalid arguments while logging in."<<endl;
			return;
		}
		s="login "+cmds[1]+" "+cmds[2]+" "+client_ip+" "+client_port;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		if(buffer[0]=='U')
		{
	
			fname_path.clear();
			downloading.clear();
			completed.clear();		
			uname=cmds[1];
			loggedin=1;
			cout<<buffer<<endl;
			vector<string> reply_tokens;
			string r=buffer;
			char *token = strtok(const_cast<char*>(r.c_str()), " ");
			while(token != NULL)
			{
				reply_tokens.push_back(token);
				token = strtok(NULL," ");
			}
			int i=1;
			while(i<reply_tokens.size())
			{
				fname_path[reply_tokens[i]]=reply_tokens[i+1];
				i=i+2;
			}
		}
		else
		{
			cout<<buffer<<endl;
		}
		return;
	}
	
	else if(cmds[0]=="logout")
	{
		if(cmds.size()!=1)
		{
			cout<<"Invalid arguments for logging out."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		s="logout "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		uname="";
		loggedin=0;
		fname_path.clear();
		completed.clear();
		return;
	}

	else if(cmds[0]=="create_group")
	{
		if(cmds.size()!=2)
		{
			cout<<"Invalid arguments for creating group."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		s="create_group "+cmds[1]+" "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;
	}
	
	else if(cmds[0]=="join_group")
	{
		if(cmds.size()!=2)
		{
			cout<<"Invalid arguments to join group."<<endl;
			return;

		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		s="join_group "+cmds[1]+" "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;	
	}
	
	else if(cmds[0]=="leave_group")
	{
		if(cmds.size()!=2)
		{
			cout<<"Invalid arguments to leave group."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		s="leave_group "+cmds[1]+" "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;			
	}
	
	else if(cmds[0]=="list_requests")
	{
		if(cmds.size()!=2)
		{
			cout<<"Invalid arguments to list requests."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}	
		s="list_requests "+cmds[1]+" "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;		
	}
	
	else if(cmds[0]=="accept_request")
	{
		if(cmds.size()!=3)
		{
			cout<<"Invalid arguments to accept request."<<endl;
			return;	
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		s="accept_request "+cmds[1]+" "+cmds[2]+" "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;			
	}
	
	else if(cmds[0]=="list_groups")
	{
		if(cmds.size()!=1)
		{
			cout<<"Invalid arguments to list groups."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		s="list_groups";
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;						
		return;
	}
	
	else if(cmds[0]=="list_files")
	{
		if(cmds.size()!=2)
		{
			cout<<"Invalid arguments to list files."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}					
		s="list_files "+cmds[1]+" "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;
	}
	
	else if(cmds[0]=="upload_file")
	{
		if(cmds.size()!=3)
		{
			cout<<"Invalid arguments to upload files."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in\n";
			return;
		}
		string filePath=cmds[1];		
		FILE* fp = fopen(filePath.c_str(),"r");   
		if(fp==NULL)
		{ 
		    cout<<"File not found."<<endl; 
		    return; 
		}
		fseek(fp,0L,SEEK_END); 
		long int file_size = ftell(fp);   
		fclose(fp);    
			
		if(file_size!=-1)
		{
			cout<<"File size = "<<file_size<<endl;
			string file_name = getFileName(filePath);
			fname_path[file_name] = filePath;
			ifstream fin;
			fin.open(filePath);
			fin.seekg(0,fin.end);
			int length=fin.tellg();
			fin.seekg(0,fin.beg);
			char* chunk=new char[length];
			fin.read(chunk,length);
			unsigned char hash[SHA_DIGEST_LENGTH]; 
			SHA1(reinterpret_cast<const unsigned char *>(chunk), sizeof(chunk) - 1, hash);
			string c_hash(reinterpret_cast<char*>(hash));
			string msg="upload_file "+cmds[2]+" "+uname+" "+file_name+" "+filePath+" "+to_string(file_size)+" "+c_hash;
			cout<<"Hash value = "<<c_hash<<endl;
			write(tracker_sock, msg.c_str() , msg.size());
			for(int i=0;i<1000;i++)
			{
				buffer[i]='\0';
			}
			int bytes_read = read(tracker_sock,buffer,1000);
			cout<<buffer<<endl;	
		}
	}
	
	else if(cmds[0] == "download_file")
	{
		if(cmds.size()!= 4)
		{
			cout<<"Invalid arguments to download file."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in"<<endl;
			return;
		}
		string file_name = cmds[2];
		string dest_path = cmds[3];
		grpid=cmds[1];
		s="download_file "+file_name+" "+cmds[1]+" "+uname;
		write(tracker_sock,s.c_str(),s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int bytes_read = read(tracker_sock,buffer,1000);
		string fileNpeersinfo = buffer;

		if(fileNpeersinfo[0]=='^')
		{
			cout<<"File not found in the group."<<endl;
			return;
		}
		if(fileNpeersinfo[0]=='#')
		{
			cout<<"You are not a member of the group."<<endl;
			return;
		}
		if(fileNpeersinfo=="~ No peers found ~")
		{
			cout<<fileNpeersinfo<<endl;
			return;
		}
		vector<string> reply_tokens;
		char *token = strtok(const_cast<char*>(fileNpeersinfo.c_str()), " ");
		while(token != NULL)
		{
			reply_tokens.push_back(token);
			token = strtok(NULL," ");
		}
		string source_path=reply_tokens[reply_tokens.size()-1];
		reply_tokens.pop_back();
		thread t(execute_download,file_name,source_path,dest_path,reply_tokens,cmds[1]);
		t.detach();
	}
	
	else if(cmds[0]=="show_downloads")
	{
		if(cmds.size()!=1)
		{
			cout<<"Invalid arguments to show downloads."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		for(auto s: completed)
		{
			cout<<"[C] "<<s<<"\n";
		}
		for(auto s: downloading)
		{
			cout<<"[D] "<<s<<"\n";
		}
	}
	
	else if(cmds[0]=="stop_share")
	{
		if(cmds.size()!=3)
		{
			cout<<"Invalid arguments to stop share."<<endl;
			return;
		}
		if(!loggedin)
		{
			cout<<"No user logged in."<<endl;
			return;
		}
		s="stop_share "+cmds[1]+" "+cmds[2]+" "+uname;
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int bytes_read = read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;	
	}
	
	else if(cmds[0]=="test")
	{
		s="test";
		write(tracker_sock, s.c_str() , s.size());
		for(int i=0;i<1000;i++)
		{
			buffer[i]='\0';
		}
		int temp_r=read(tracker_sock,buffer,1000);
		cout<<buffer<<endl;
		return;
	}
	
	else if(cmds[0]=="quit")
	{
		cout<<"Closing the client....."<<endl;
		exit(0);
	}
	
	else
	{
		cout<<"Invalid command"<<endl;
	}
}

int main(int argc, char const *argv[]) 
{ 
	if(argc != 5)
	{
		cout<<"Invalid Arguments\n";
		return 1;
	}

	loggedin = 0;
	string tracker_port=argv[4];
	string tracker_ip=argv[3];
	string client_port=argv[2];
	string client_ip=argv[1];
	
	cout<<client_ip<<":"<<client_port<<" "<<tracker_ip<<":"<<tracker_port<<endl;

	tracker_sock = 0; 
	struct sockaddr_in serv_addr; 
	if((tracker_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{ 
		cout<<endl<<"Error: Socket creation failed."<<endl; 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(stoi(tracker_port.c_str())); 
	
	if(inet_pton(AF_INET, tracker_ip.c_str(), &serv_addr.sin_addr)<=0)
	{ 
		cout<<endl<<"Error: Invalid Address"<<endl; 
		return -1; 
	}
	
	if (connect(tracker_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{ 
		cout<<endl<<"Error: Connection failed."<<endl; 
		return -1; 
	}
	
	fname_path.clear();
	thread th(serve_to_peers,client_ip,client_port);
	while(1)
	{
		connect_with_tracker(client_ip,client_port);
	}
	th.join();
	return 0; 
}
