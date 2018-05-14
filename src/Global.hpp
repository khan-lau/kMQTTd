//
//  Global.hpp
//  MQTTd
//
//  Created by Khan on 16/4/13.
//  Copyright © 2016年 Khan. All rights reserved.
//

#ifndef __GLOBAL_HPP__
#define __GLOBAL_HPP__


#include "ChTree.hpp"
#include "CppUtils.hpp"

class Global {

private:
    void init(){
    
    }
    
    
    ChTree & getRoot(){
        if (this->_pRoot == nullptr) {
//            this->initRoot();
        }
        
        return *(this->_pRoot);

    }
    
    
private:
    std::shared_ptr<ChTree> _pRoot; //频道根节点
    
    

};


#endif /* __GLOBAL_HPP__ */
