//
//  CppUtils.hpp
//  MQTTd
//
//  Created by Khan on 16/4/25.
//  Copyright © 2016年 Khan. All rights reserved.
//

#ifndef __CppUtils_HPP__
#define __CppUtils_HPP__

#include <string>
#include <memory>
#include <vector>
#include <iterator>



typedef std::vector<std::string> VecStrs;



class CppUtils{

public:

    
    inline static std::string trim(std::string text) {
        if( ! text.empty() ) {
            text.erase( 0, text.find_first_not_of((" \n\r\t")) );
            text.erase( text.find_last_not_of((" \n\r\t")) + 1 );
        }
        return text;
    }
    
    /**
     * @brief 将字符串以"delim"为分隔符, 切割成数组
     * 
     * @param s 源字符串
     * @param delim 分隔符
     * @return std::shared_ptr<VecStrs> 返回的数组
     */
    static std::shared_ptr<VecStrs> split( const std::string& s, const std::string& delim = "," ) {
        size_t last = 0;
        std::shared_ptr<VecStrs> p_vec = std::make_shared<VecStrs>();
        
        size_t index = s.find_first_of( delim, last );
        while ( index != std::string::npos ) {
            p_vec->push_back( s.substr(last, index - last) );
            last = index + 1;
            index = s.find_first_of(delim, last);
            
        }
        
        if ( index > s.length() && index - last > 0 ) {
            p_vec->push_back( s.substr(last, index - last ) );
        }
        
        return p_vec;
    }
    
    /**
     * @brief 判断字符串是否以"delim"开头
     * 
     * @param s 源字符串
     * @param delim 开头匹配字符串
     * @return true 匹配
     * @return false 不匹配
     */
    static bool startWith(const std::string& s, const std::string& delim){
        if(s.empty()) return false;
        size_t index = s.find_first_of( delim, 0 );
        if ( index != std::string::npos && index == 0 ) {
            return true;
        }
        return false;
    }
    
    /**
     * @brief 判断字符串是否已"delim"结尾
     * 
     * @param s 源字符串
     * @param delim 结尾匹配字符串
     * @return true 匹配
     * @return false 不匹配
     */
    static bool endWith(const std::string& s, const std::string& delim){
        if(s.empty()) return false;
        size_t index = s.find_last_of( delim );
        if ( index != std::string::npos && index == s.length() - 1 ) {
            return true;
        }
        return false;
    }
    
    static std::shared_ptr<VecStrs> splitChannel( const std::string& s) {
        const std::string& delim = "/" ;
        size_t last = 0;
        
        std::shared_ptr<VecStrs> p_vec = std::make_shared<VecStrs>();
        
        size_t index = s.find_first_of( delim, last );
        if ( index != std::string::npos && index == 0 ) {
            last = index + 1;
            index = s.find_first_of( delim, last );
            if ( index != std::string::npos) {
                p_vec->push_back( s.substr(0, index) );
                
                last = index + 1;
                index = s.find_first_of(delim, last);
                while ( index != std::string::npos ) {
                    if(index != last){
                        p_vec->push_back( s.substr(last, index - last) );
                    }
                    last = index + 1;
                    index = s.find_first_of(delim, last);
                }
                
                if ( index > s.length()  && index - last > 0 ) {
                    p_vec->push_back( s.substr(last, index - last ) );
                }
                
            } else {

                if ( index > s.length()  && index - last > 0 ) {
                    p_vec->push_back( s.substr(last, index - last ) );
                }
            }
            
        } else {
            while ( index != std::string::npos ) {
                if(index != last){
                    p_vec->push_back( s.substr(last, index - last) );
                }
                last = index + 1;
                index = s.find_first_of(delim, last);
            }
            
            if ( index > s.length() && index - last > 0 ) {
                p_vec->push_back( s.substr(last, index-last ) );
            }
        }

        return p_vec;
    }

};


/**
 * @brief 用智能指针shared_ptr管理一个动态数组
 * 
 * @tparam T 数组元素类型
 * @param size 数组大小
 * @return shared_ptr<T> 
 */
template <typename T>  
shared_ptr<T> make_shared_array(size_t size)  
{  
    //default_delete是STL中的默认删除器  
    return shared_ptr<T>( new T[size], default_delete<T[]>() );  
}  


/**
 * @brief 反序base-range for
 * 
 * @tparam C 
 */
template <typename C>
struct reverse_wrapper {

    C & c_;
    reverse_wrapper(C & c) :  c_(c) {}
    
    typename C::reverse_iterator begin() {return c_.rbegin();}
    typename C::reverse_iterator end() {return c_.rend(); }
};

template <typename C, size_t N>
struct reverse_wrapper< C[N] >{
    
    C (&c_)[N];
    reverse_wrapper( C(&c)[N] ) : c_(c) {}
    
    typename std::reverse_iterator<const C *> begin() { return std::rbegin(c_); }
    typename std::reverse_iterator<const C *> end() { return std::rend(c_); }
};


template <typename C>
reverse_wrapper<C> r_wrap(C & c) {
    return reverse_wrapper<C>(c);
}





/**
 * @brief 单例模板
 * 
 * @tparam T 单例对象的类型
 */
template <typename T>
class Singleton {
public:
    template <typename... Args>
    static T& getInstance(Args&&... args) {
        static T instance(std::forward<Args>(args)...);
        return instance;
    }
};



#endif /* __CppUtils_HPP__ */
