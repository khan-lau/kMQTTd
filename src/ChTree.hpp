//
//  ChTree.h
//  MQTTd
//
//  Created by Khan on 16/4/20.
//  Copyright © 2016年 Khan. All rights reserved.
//

#ifndef __CH_TREE__
#define __CH_TREE__

#include <iostream>

#include "VarTypes.h"
#include "Protocol.hpp"
#include "CppUtils.hpp"

using std::list;
using std::make_pair;



struct ChNode;
struct User;

typedef std::shared_ptr<ChNode> PNode;
typedef std::weak_ptr<ChNode>   Weak_PNode;

typedef std::shared_ptr<User>   PUser;
typedef std::weak_ptr<User>     Weak_PUser;



/**用于记录channel节点的多叉树
 */

struct ChNode {

    
    std::string      name;
//    Uint             retian;            //订阅的引用计数引用计数
    
    std::list<PNode> nodes;             //子节点
    Uint node_size;                     //子节点数.. list的size代价为O(N), 所以单独维护一个size, 新channel节点创建+1 删除-1, 节点数为0, 则考虑删除
    
    std::list<PMessage> messages;
    Uint msg_size;                      //消息数量
    
    std::list<Weak_PUser> users;
    Uint user_size;                     //用户数, 订阅+1, 取消订阅-1, 链接断开当做取消订阅
    
    
public:
    friend std::ostream& operator<<(std::ostream& out, const ChNode& node){
        out << "{\"name\" : \"" << node.name << "\" , \"node_size\" : " << node.node_size << ", \"nodes\" :";
        
        if ( node.nodes.empty() ) {
            out << "null";
        } else {
            out << "[";
            for (PNode p_node : node.nodes) {
                out << *p_node ;
                if (p_node != node.nodes.back()) {
                    out << ", ";
                }
            }
            out << "]";
        }
        out << ", \"msg_size\" : " << node.msg_size;
        out << ", \"user_size\" : " << node.user_size;
        out << "}";

        return out;
    }
    
};

class ChTree {
    
public:
    ChTree() {
        initRoot();
    }
    

public:
    std::shared_ptr<ChNode> _pRoot; //频道根节点
    
private:
    
    void initRoot(){
        if (this->_pRoot == nullptr) {
            this->_pRoot = make_shared<ChNode>();
            
            this->_pRoot->name = "/";
            this->_pRoot->node_size = 0;
            this->_pRoot->msg_size = 0;
            this->_pRoot->user_size = 0;
        }
    }
    
    /**递归进行频道树结构寻路径
     */
    PNode findNode(const shared_ptr<VecStrs> p_vec, Uint pos, const PNode &node){
        if(pos < p_vec->size() ){
            string str = (*p_vec)[pos];
            auto ite = std::find_if(std::begin(node->nodes), std::end(node->nodes), [str](PNode& cnode)->bool{
                if(cnode == nullptr) return false;
                return str == cnode->name;
            });
            
            //如果当前层未找到对应的node
            if( ite == std::end(node->nodes) ) {
                if (node == this->_pRoot)
                    return nullptr;
                else
                    return node;
            } else {
                int n = pos + 1;
                if( n < p_vec->size() ){
                    return findNode(p_vec, n, *ite);
                } else {
                    return *ite;
                }
            }
            
            return *ite;
        }
        return nullptr;
    }
    

    
public:

    
    bool isChannel(string channel) {
        //channel合法性检查
        if (channel.length() > 0xFFFF) return false;
        
        //channel分段不符合规范
        size_t pos = channel.find("//");
        if (pos != std::string::npos) return false;
        
        //channel为空
        string str = CppUtils::trim(channel);
        if(str.empty()) return false;
        
//        if(str == "/") return false;  //协议要求允许"/" 这样的topic [MQTT-4.7.3]
        
        //包含#通配符, 但是不在最尾端, 它都必须是主题过滤器的最后一个字符 [MQTT-4.7.1-2]
        string key = "#";
        pos = str.find(key);
        if (pos != std::string::npos && pos != str.length() - key.length() ) return false;
        
        if (pos != std::string::npos && pos > 0 && str[pos-1] != '/') return false;//如果通配符'#'前面有字符, 该字符必须是'/'
        
        key = "+";
        pos = str.find(key);
        if (pos != std::string::npos && pos > 0 && str[pos-1] != '/') return false; //如果通配符'+'前面有字符, 该字符必须是'/'
        if (pos != std::string::npos && pos < str.length() - key.length() && str[pos+1] != '/') return false; //如果通配符'+'后面有字符, 该字符必须是'/'
        
        return true;
    }
    
    

    /** 根据channel路径找到对应的node, 暂时废弃
     */
    PNode findNode(string channel){
        if ( ! isChannel(channel) ) return nullptr;
        
        auto p_vec = CppUtils::splitChannel(channel);
        
        return findNode(p_vec, 0, this->_pRoot);
    }
    
    /** 根据channel路径找到对应的node
     */
    PNode findChannel(string channel){
        if ( ! isChannel(channel) ) { //channel不合法
            return nullptr;
        }
        auto p_vec = CppUtils::splitChannel(channel);
        PNode p_node = this->_pRoot;
        for ( string str : *p_vec) {
            auto ite = std::find_if(std::begin(p_node->nodes), std::end(p_node->nodes), [str](PNode& cnode)->bool{
                if(cnode == nullptr) return false;
                return str == cnode->name;
            });
            
            if (ite != std::end(p_node->nodes)) { //如果没找到
                if (p_node == this->_pRoot) return nullptr;
                else return p_node;
            } else {
                p_node = *ite;
            }
        }
        
        return p_node;
    }
    
    /** 添加路径
     */
    void addChannel(std::string channel) {
        if ( ! isChannel(channel) ) { //channel不合法
            return ;
        }
        auto p_vec = CppUtils::splitChannel(channel);
        
        PNode p_node = this->_pRoot;
        for ( string str : *p_vec) {
            
            auto ite = std::find_if(std::begin(p_node->nodes), std::end(p_node->nodes), [str](PNode& cnode)->bool{
//                if(cnode == nullptr) return false;
                return str == cnode->name;
            });

            if (ite == std::end(p_node->nodes)) { //如果没找到, 则新建一个node, 加入tree中
                PNode new_node = make_shared<ChNode>();
                new_node->name = str;
                new_node->node_size = 0;
                new_node->msg_size = 0;
                new_node->user_size = 0;
                
                p_node->node_size += 1;
                p_node->nodes.push_back(new_node);
                
                p_node = new_node;
            } else { //如果找到
                p_node = *ite;
            }
        }
    }
    
    
    /** 从频道树结构中删除一个channel
     */
    void removeChannel(std::string channel) {
        if ( ! isChannel(channel) ) { //channel不合法
            return ;
        }
        auto p_vec = CppUtils::splitChannel(channel);
        
        list<PNode> nodes;
        PNode p_node = this->_pRoot;
        nodes.push_back(p_node);
        
        for ( string str : *p_vec) {
            
            auto ite = std::find_if(std::begin(p_node->nodes), std::end(p_node->nodes), [str](PNode& cnode)->bool{
                if(cnode == nullptr) return false;
                return str == cnode->name;
            });
            
            if (ite != std::end(p_node->nodes)) { //如果没找到
                p_node = *ite;
                
                if ( p_node != nullptr ) {
                    nodes.push_back(p_node);
                }
                
            } else { //如果找到
                break;
            }
        }
        
        PNode p_child = nullptr;
        for (PNode pn : r_wrap(nodes) ) {
            if( pn != nodes.back() ) {

                if (p_child != nodes.back() ) { //如果不是最末端节点
                    if ( p_child->node_size == 0 && p_child->user_size == 0) { //如果当前节点无子节点, 并且订阅用户数为0
                        pn->node_size -= 1;
                        pn->nodes.remove(p_child);
                    }
                } else {
                    pn->node_size -= 1;
                    pn->nodes.remove(p_child);
                }
            }
            
            p_child = pn;
        }
    }
    
    
    
};





struct User {
    
    std::string user;
    std::string passwd;
    std::string token;
    
    std::list< Weak_PNode > channels;
};

#endif /* __CH_TREE__ */
