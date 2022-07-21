/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition (Revised 2020)
 *
 * MP2 Starter template version
 **********************************/
#include "MP2Node.h"
#include "Message.h"
#include "common.h"
#include <cstdio>
#include <string>

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
	this->delimiter = "::";
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
}

/**
* FUNCTION NAME: updateRing
*
* DESCRIPTION: This function does the following:
*                 1) Gets the current membership list from the Membership Protocol (MP1Node)
*                    The membership list is returned as a vector of Nodes. See Node class in Node.h
*                 2) Constructs the ring based on the membership list
*                 3) Calls the Stabilization Protocol
*/
void MP2Node::updateRing() {
	/*
     * Implement this. Parts of it are already implemented
     */
    vector<Node> curMemList;
    bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());

    if(curMemList.size() != ring.size()){
        change = true;
    }else{
        for(int i=0; i<curMemList.size(); i++){
            if(curMemList[i].getHashCode() != ring[i].getHashCode()){
                change = true;
                break;
            }
        }
    }
    if(change){
        ring = curMemList;
        stabilizationProtocol();
    }
	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
}

/**
* FUNCTION NAME: getMembershipList
*
* DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
*                 i) generates the hash code for each member
*                 ii) populates the ring member in MP2Node class
*                 It returns a vector of Nodes. Each element in the vector contain the following fields:
*                 a) Address of the node
*                 b) Hash code obtained by consistent hashing of the Address
*/
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
* FUNCTION NAME: hashFunction
*
* DESCRIPTION: This functions hashes the key and returns the position on the ring
*                 HASH FUNCTION USED FOR CONSISTENT HASHING
*
* RETURNS:
* size_t position on the ring
*/
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
* FUNCTION NAME: clientCreate
*
* DESCRIPTION: client side CREATE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
// message types, reply is the message from node to coordinator
enum MessageType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY};
// enum of replica types
enum ReplicaType {PRIMARY, SECONDARY, TERTIARY};
Message(int _transID, Address _fromAddr, MessageType _type, string _key, string _value, ReplicaType _replica)
*/
void MP2Node::clientCreate(string key, string value) {
	/*
	* Implement this
	*/
    g_transID++;
    
    vector<Node> nodes = findNodes(key);
     //log->logA(&memberNode->addr, memberNode->addr.getAddress(), nodes[0].getAddress()->getAddress(), nodes[1].getAddress()->getAddress(), nodes[2].getAddress()->getAddress());
     //log->logRing(&memberNode->addr, ring);

    for(int i=0; i<nodes.size(); i++){
        Message m(g_transID, memberNode->addr, CREATE, key, value, static_cast<ReplicaType>(i));
        emulNet->ENsend(&memberNode->addr, nodes[i].getAddress(), m.toString());
    }
    Message m(g_transID, memberNode->addr, CREATE, key, value, PRIMARY);
    transMap.insert(std::pair<int, Message>(g_transID, m));
}

/**
* FUNCTION NAME: clientRead
*
* DESCRIPTION: client side READ API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
Message(int _transID, Address _fromAddr, MessageType _type, string _key)
*/
void MP2Node::clientRead(string key){
	/*
	* Implement this
	*/
    //log->logString(&memberNode->addr, "READING!!!!");
    g_transID++;
    vector<Node> repNodes = findNodes(key);
    Message m(g_transID, memberNode->addr, READ, key);
    
    emulNet->ENsend(&memberNode->addr, repNodes[0].getAddress(), m.toString());
    emulNet->ENsend(&memberNode->addr, repNodes[1].getAddress(), m.toString());
    emulNet->ENsend(&memberNode->addr, repNodes[2].getAddress(), m.toString());

    transMap.insert(std::pair<int, Message>(g_transID, m));
}

/**
* FUNCTION NAME: clientUpdate
*
* DESCRIPTION: client side UPDATE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientUpdate(string key, string value){
	/*
    * Implement this
    */
    g_transID++;
    Message m1(g_transID, memberNode->addr, UPDATE, key, value, PRIMARY);
    Message m2(g_transID, memberNode->addr, UPDATE, key, value, SECONDARY);
    Message m3(g_transID, memberNode->addr, UPDATE, key, value, TERTIARY);
    

    vector<Node> repNodes = findNodes(key);

    emulNet->ENsend(&memberNode->addr, repNodes[0].getAddress(), m1.toString());
    emulNet->ENsend(&memberNode->addr, repNodes[1].getAddress(), m2.toString());
    emulNet->ENsend(&memberNode->addr, repNodes[2].getAddress(), m3.toString());
    transMap.insert(std::pair<int, Message>(g_transID, m1));
}

/**
* FUNCTION NAME: clientDelete
*
* DESCRIPTION: client side DELETE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientDelete(string key){
	/*
	* Implement this
	*/
    g_transID++;
    vector<Node> repNodes = findNodes(key);
    for(int i=0; i<repNodes.size(); i++){
        Message m(g_transID, memberNode->addr, DELETE, key);
        emulNet->ENsend(&memberNode->addr, repNodes[i].getAddress(), m.toString());
    }
    Message m(g_transID, memberNode->addr, DELETE, key);
    transMap.insert(std::pair<int, Message>(g_transID, m));
}

/**
* FUNCTION NAME: createKeyValue
*
* DESCRIPTION: Server side CREATE API
*                    The function does the following:
*                    1) Inserts key value into the local hash table
*                    2) Return true or false based on success or failure
*/
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
    if(!ht->read(key).empty()){
        return false;
    }
    Entry e(value, par->getcurrtime(), replica);
    bool status = ht->create(key, e.convertToString());
    vector<Node> nodes = findNodes(key);
    
    if(replica == PRIMARY){
        hasMyReplicas.clear();
        hasMyReplicas.push_back(nodes[0]);
        hasMyReplicas.push_back(nodes[1]);
        hasMyReplicas.push_back(nodes[2]);
    }
    if(replica == TERTIARY){
        haveReplicasOf.clear();
	    haveReplicasOf.push_back(nodes[0]);
	    haveReplicasOf.push_back(nodes[1]);
        haveReplicasOf.push_back(nodes[2]);
    }
    return status;
}

/**
* FUNCTION NAME: readKey
*
* DESCRIPTION: Server side READ API
*                 This function does the following:
*                 1) Read key from local hash table
*                 2) Return value
*/
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
    
    return ht->read(key);
}

/**
* FUNCTION NAME: updateKeyValue
*
* DESCRIPTION: Server side UPDATE API
*                 This function does the following:
*                 1) Update the key to the new value in the local hash table
*                 2) Return true or false based on success or failure
*/
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
    Entry e(value, par->getcurrtime(), replica);
    return ht->update(key, e.convertToString());
}

/**
* FUNCTION NAME: deleteKey
*
* DESCRIPTION: Server side DELETE API
*                 This function does the following:
*                 1) Delete the key from the local hash table
*                 2) Return true or false based on success or failure
*/
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table
    return ht->deleteKey(key);
}

/**
* FUNCTION NAME: checkMessages
*
* DESCRIPTION: This function is the message handler of this node.
*                 This function does the following:
*                 1) Pops messages from the queue
*                 2) Handles the messages according to message types
*/
void MP2Node::checkMessages() {
	/*
	* Implement this. Parts of it are already implemented
	*/
	char * data;
	int size;

	/*
	* Declare your local variables here
	*/

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);
        Message msg(message);
		/*
		 * Handle the message types here
		 */
         
        //log->logString(&memberNode->addr, std::to_string(msg.type));
        switch (msg.type) {
            case CREATE:
                        onCreate(msg);
                        break;
            case READ:  
                        onRead(msg);
                        break;
            case UPDATE:
                        onUpdate(msg);
                        break;
            case DELETE:
                        onDelete(msg);
                        break;
            case REPLY:{
                        MessageType t = transMap.find(msg.transID)->second.type;
                        if(t == UPDATE)
                            onUpdateReplyMsg(msg);
                        else if(t == CREATE)
                            onCreateReplyMsg(msg);
                        else if(t == DELETE)
                            onDeleteReplyMsg(msg);
                        break;          
            }
            case READREPLY:
                        onReadReplyMsg(msg);
                        break;
            default:
                    std::cout<<"UNKNOWN Message Type --- you're screwed";
        }
	}

	/*
	* This function should also ensure all READ and UPDATE operation
	* get QUORUM replies
	*/
}
//Address * address, bool isCoordinator, int transID, string key, string value
void MP2Node::onCreate(Message & msg){

    bool status = createKeyValue(msg.key, msg.value, msg.replica);

    if(status){
        log->logCreateSuccess(&memberNode->addr, false, msg.transID, msg.key, msg.value);
    }else{
        log->logCreateFail(&memberNode->addr, false, msg.transID, msg.key, msg.value);
    }

    Message rep_msg(msg.transID, memberNode->addr, REPLY, status);
	emulNet->ENsend(&memberNode->addr, &msg.fromAddr, rep_msg.toString());

}


void MP2Node::onRead(Message & msg){
    string value = readKey(msg.key);

    if(!value.empty()){
        log->logReadSuccess(&memberNode->addr, false, msg.transID, msg.key, value);
    }else{
        log->logReadFail(&memberNode->addr, false, msg.transID, msg.key);
    }
    
    Message readMsg(msg.transID, memberNode->addr, value);
    emulNet->ENsend(&memberNode->addr, &msg.fromAddr, readMsg.toString());
}
void MP2Node::onDelete(Message & msg){
    bool status = ht->deleteKey(msg.key);


    if(status){
        log->logDeleteSuccess(&memberNode->addr, false, msg.transID, msg.key);
    }else{
        log->logDeleteFail(&memberNode->addr, false, msg.transID, msg.key);
    }
    
    Message rep_msg(msg.transID, memberNode->addr, REPLY, status);
	emulNet->ENsend(&memberNode->addr, &msg.fromAddr, rep_msg.toString());

}

void MP2Node::onUpdate(Message & msg){
    bool status = updateKeyValue(msg.key, msg.value, msg.replica);
    
    if(status){
        log->logUpdateSuccess(&memberNode->addr, false, msg.transID, msg.key, msg.value);
    }else{
        log->logUpdateFail(&memberNode->addr, false, msg.transID, msg.key, msg.value);
    }
    
    Message updateMsg(msg.transID, memberNode->addr, REPLY, status);
    emulNet->ENsend(&memberNode->addr, &msg.fromAddr, updateMsg.toString());

}
void MP2Node::onCreateReplyMsg(Message & msg){
    
    Message orig = transMap.find(msg.transID)->second;
    std::map<int,counts>::iterator it = replyCountMap.find(msg.transID);
    if(it != replyCountMap.end()){
        if(replyCountMap.find(msg.transID)->second.success > 0 && msg.success){
            log->logCreateSuccess(&memberNode->addr, true, msg.transID, orig.key, orig.value);
            replyCountMap.erase(it);
        }
        else if(replyCountMap.find(msg.transID)->second.failed > 0 && !msg.success){
            log->logCreateFail(&memberNode->addr, true, msg.transID, orig.key, orig.value);
            replyCountMap.erase(it);
        }
        else{
            if(msg.success){
                it->second.success++;
            }else{
                it->second.failed++;
            }
        }
    }else{
        counts c;
        if(msg.success){
            c.success++;
        }else{
            c.failed++;
        }
        replyCountMap.insert(std::pair<int,counts>(msg.transID, c));
    }
}
void MP2Node::onReadReplyMsg(Message &msg){
    std::map<int,counts>::iterator it = replyCountMap.find(msg.transID);
    Message orig = transMap.find(msg.transID)->second;

    int failedCount = 0;
    vector<Node> replicas = findNodes(orig.key);
    for(int i=0; i<3; i++){
        if(!nodeStatus(replicas[i].nodeAddress))
            failedCount++;
    }
    log->logString(&memberNode->addr, std::to_string(failedCount));
    if(it != replyCountMap.end()){
        if(replyCountMap.find(msg.transID)->second.success > 0 && !msg.value.empty()){
            log->logReadSuccess(&memberNode->addr, true, msg.transID, orig.key, msg.value);
            replyCountMap.erase(it);
        }
        else if(replyCountMap.find(msg.transID)->second.failed > 0 && msg.value.empty()){
            log->logReadFail(&memberNode->addr, true, msg.transID, orig.key);
            replyCountMap.erase(it);
        }
        else{
            if(!msg.value.empty()){
                it->second.success++;
            }else{
                it->second.failed++;
            }
        }
    }else{
        counts c;
        if(!msg.value.empty()){
            c.success++;
        }else{
            c.failed++;
        }
        replyCountMap.insert(std::pair<int,counts>(msg.transID, c));
    }
}
void MP2Node::onUpdateReplyMsg(Message &msg){
    Message orig = transMap.find(msg.transID)->second;
    std::map<int,counts>::iterator it = replyCountMap.find(msg.transID);
    if(it != replyCountMap.end()){
        if(replyCountMap.find(msg.transID)->second.success > 0 && msg.success){
            log->logUpdateSuccess(&memberNode->addr, true, msg.transID, orig.key, orig.value);
            replyCountMap.erase(it);
        }
        else if(replyCountMap.find(msg.transID)->second.failed > 0 && !msg.success){
            log->logUpdateFail(&memberNode->addr, true, msg.transID, orig.key, orig.value);
            replyCountMap.erase(it);
        }
        else{
            if(msg.success){
                it->second.success++;
            }else{
                it->second.failed++;
            }
        }
    }else{
        counts c;
        if(msg.success){
            c.success++;
        }else{
            c.failed++;
        }
        replyCountMap.insert(std::pair<int,counts>(msg.transID, c));
    }
}
void MP2Node::onDeleteReplyMsg(Message & msg){
    Message orig = transMap.find(msg.transID)->second;
    std::map<int,counts>::iterator it = replyCountMap.find(msg.transID);
    if(it != replyCountMap.end()){
        if(replyCountMap.find(msg.transID)->second.success > 0 && msg.success){
            log->logDeleteSuccess(&memberNode->addr, true, msg.transID, orig.key);
            replyCountMap.erase(it);
        }
        else if(replyCountMap.find(msg.transID)->second.failed > 0 && !msg.success){
            log->logDeleteFail(&memberNode->addr, true, msg.transID, orig.key);
            replyCountMap.erase(it);
        }
        else{
            if(msg.success){
                it->second.success++;
            }else{
                it->second.failed++;
            }
        }
    }else{
        counts c;
        if(msg.success){
            c.success++;
        }else{
            c.failed++;
        }
        replyCountMap.insert(std::pair<int,counts>(msg.transID, c));
    }
}
/**
* FUNCTION NAME: findNodes
*
* DESCRIPTION: Find the replicas of the given keyfunction
*                 This function is responsible for finding the replicas of a key
*/
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i < (int)ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
	if ( memberNode->bFailed ) {
		return false;
	}
	else {
		return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
	}
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
* FUNCTION NAME: stabilizationProtocol
*
* DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
*                 It ensures that there always 3 copies of all keys in the DHT at all times
*                 The function does the following:
*                1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
*                Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
*/
void MP2Node::stabilizationProtocol() {
	/*
	* Implement this
	*/
   
    
    for(auto& entry : ht->hashTable){
        Entry e(entry.second);
        vector<Node> replicas = findNodes(entry.first);
        auto inReplicas = find_if(replicas.begin(),
            replicas.end(),
            [&](Node &n){return n.nodeAddress == memberNode->addr;});
        if (inReplicas != replicas.end()) {
            switch (e.replica) {
                case PRIMARY:{
                    bool second = nodeStatus(replicas[1].nodeAddress);
                    bool third = nodeStatus(replicas[2].nodeAddress);
                    if(!second || !third){
                        g_transID++;
                    }
                    if(!second){
                        Message m(g_transID, memberNode->addr, CREATE, entry.first, entry.second, SECONDARY);
                        emulNet->ENsend(&memberNode->addr, replicas[1].getAddress(), m.toString());
                        transMap.insert(std::pair<int, Message>(g_transID, m));
                    }
                    if(!third){
                        Message m(g_transID, memberNode->addr, CREATE, entry.first, entry.second, TERTIARY);
                        emulNet->ENsend(&memberNode->addr, replicas[2].getAddress(), m.toString());
                        transMap.insert(std::pair<int, Message>(g_transID, m));
                    }
                    break;
                }
                case SECONDARY:{
                    bool first = nodeStatus(replicas[0].nodeAddress);
                    bool third = nodeStatus(replicas[2].nodeAddress);
                    if(!first || !third){
                        g_transID++;
                    }
                    if(!first){
                        Message m(g_transID, memberNode->addr, CREATE, entry.first, entry.second, PRIMARY);
                        emulNet->ENsend(&memberNode->addr, replicas[0].getAddress(), m.toString());
                        transMap.insert(std::pair<int, Message>(g_transID, m));
                    }
                    if(!third){
                        Message m(g_transID, memberNode->addr, CREATE, entry.first, entry.second, TERTIARY);
                        emulNet->ENsend(&memberNode->addr, replicas[2].getAddress(), m.toString());
                        transMap.insert(std::pair<int, Message>(g_transID, m));
                    }
                    break;
                }
                case TERTIARY:{
                    bool first = nodeStatus(replicas[0].nodeAddress);
                    bool second = nodeStatus(replicas[1].nodeAddress);
                    if(!first || !second){
                        g_transID++;
                    }
                    if(!first){
                        Message m(g_transID, memberNode->addr, CREATE, entry.first, entry.second, PRIMARY);
                        emulNet->ENsend(&memberNode->addr, replicas[0].getAddress(), m.toString());
                        transMap.insert(std::pair<int, Message>(g_transID, m));
                    }
                    if(!second){
                        Message m(g_transID, memberNode->addr, CREATE, entry.first, entry.second, SECONDARY);
                        emulNet->ENsend(&memberNode->addr, replicas[1].getAddress(), m.toString());
                        transMap.insert(std::pair<int, Message>(g_transID, m));
                    }
                    break;
                }
            }
        }else{
            g_transID++;
            Message m(g_transID, memberNode->addr, CREATE, entry.first, entry.second, PRIMARY);
            emulNet->ENsend(&memberNode->addr, replicas[0].getAddress(), m.toString());
                    
            Message m2(g_transID, memberNode->addr, CREATE, entry.first, entry.second, SECONDARY);
            emulNet->ENsend(&memberNode->addr, replicas[1].getAddress(), m2.toString());
                
            Message m3(g_transID, memberNode->addr, CREATE, entry.first, entry.second, TERTIARY);
            emulNet->ENsend(&memberNode->addr, replicas[2].getAddress(), m3.toString());
            transMap.insert(std::pair<int, Message>(g_transID, m));
            ht->hashTable.erase(entry.first);
        }
        // if(e.replica == PRIMARY){
        //     vector<Node> nodes = findNodes(entry.first);
        //     if(ring[(index+1)%ring.size()].getAddress() != hasMyReplicas[0].getAddress()){
        //         Message m(g_transID, memberNode->addr, CREATE, entry.first, e.value, SECONDARY);
        //         emulNet->ENsend(&memberNode->addr, ring[(index+1)%ring.size()].getAddress(), m.toString());
        //     }
        //     if(ring[(index+2)%ring.size()].getAddress() != hasMyReplicas[1].getAddress()){
        //         Message m(g_transID, memberNode->addr, CREATE, entry.first, e.value, TERTIARY);
        //         emulNet->ENsend(&memberNode->addr, ring[(index+2)%ring.size()].getAddress(), m.toString());
        //     }
        //     if(ring[(index+3)%ring.size()].getAddress() == hasMyReplicas[2].getAddress()){
        //         Message m(g_transID, memberNode->addr, DELETE, entry.first, e.value);
        //         emulNet->ENsend(&memberNode->addr, ring[(index+3)%ring.size()].getAddress(), m.toString());
        //     }
        // }
}
    // hasMyReplicas.clear();
    // hasMyReplicas.push_back(ring[(index+1)%ring.size()]);
    // hasMyReplicas.push_back(ring[(index+2)%ring.size()]);
    // hasMyReplicas.push_back(ring[(index+3)%ring.size()]);
}
bool MP2Node::nodeStatus(Address & adr)
{
    auto memberList = getMembershipList();
    for(auto &&node: memberList)
    {
        if(node.nodeAddress == adr)
            return true;
    }
    return false;
}

