# Peer-to-Peer Group Based File Sharing System

### Prerequisites
Socket Programming, SHA1 hash, Multi-threading in C++
### Goal
Build a group based file sharing system where users can share, download files from the group they belong to. Download should be parallel with multiple? pieces from multiple peers.

### Architecture Overview:
The Following entities will be present in the network :<br/>
**1. Tracker:**<br/>
     Maintain information of clients with their files(shared by client) to assist the clients for the communication between peers<br/>
**2. Clients:**<br/>
Retrieve peer information from tracker for the file and download file from multiple peers simultaneously and all the files which client downloads will be shareable to other users in the same group.<br/>

### How to compile project
1. Go to tracker directory
   * ```g++ tracker.cpp -o tracker -lpthread```
2. Go to client directory
   * ```g++ client.cpp -o client -lssl -lcrypto -lpthread```


### How to Run project
#### To run the Server
```
./tracker <tracker_ip> <tracker_port>
eg : ./tracker 127.0.0.1 8000
```
#### To run the Client

```
./client <client_ip> <client_port> <tracker_ip> <tracker_port>
```
* creating client1 on new terminal with socket : 127.0.0.1:8000 <br/>
eg : ```./client 127.0.0.1 3000 127.0.0.1 8000```

#### Commands on the Client side 
 **1. Create User Account :** 
 ```
 create_user <user_id> <passwd>
 ```
 **2. Login :**
 ```
 login <user_id> <passwd>
 ```
 **3. Create Group  :**
 ```
 create_group <group_id>
 ```
 **4. Join Group :**
 ```
 join_group <group_id>
 ```
 **5. Leave Group  :**
 ```
 leave_group <group_id>
 ```
 **6. List Pending Request :**
 ```
 requests list_requests <group_id>
 ```
 **7. Accept Group Joining Request :**
 ```
 accept_request <group_id> <user_id>
 ```
 **8. List All Group In Network :**
 ```
 list_groups
 ```
 **9. List All sharable Files In Group :**
 ```
 list_files <group_id>
 ```
 **10. Upload File :**
 ```
 upload_file <file_path> <group_id>
 NOTE: Enter absolute path of the file.
 ```
 **11. Download File :**
 ```
 download_file <group_id> <file_name> <destination_path>
 NOTE: Enter absolute path of the destination.
 ```
 **12. Logout :**
 ```
 logout
 ```
 **13. Show_downloads :**
 ```
 show_downloads
 ```
 Output format:
[D] [grp_id] filename
[C] [grp_id] filename
D(Downloading), C(Complete)
</br></br>
 **14. Stop sharing :**
 ```
 stop_share <group_id> <file_name>
 ``` 

   
