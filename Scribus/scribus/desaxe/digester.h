/*
 *  digester.h
 *  
 *
 *  Created by Andreas Vox on 02.06.06.
 *  Copyright 2006 under GPL2. All rights reserved.
 *
 */



#ifndef DIGESTER_H
#define DIGESTER_H

#include <cassert>
#include <iostream>
#include <vector>
#include <map>
#include <string>

#include "desaxe_conf.h"
#include "saxhandler.h"

namespace desaxe {
	
class Action;
class RuleState;


/**
   Digester helps you to create C++ objects from a SAX stream.
   All you have to do is register actions for patterns of element tags
   you expect in your XML stream. These actions can create new objects, set
   attributes, or call methods. Actions operate on intermediate objects which 
   are hold on a stack maintained by Digester.  
 */
class Digester : public SaxHandler {	
public:
	Digester();
	Digester& operator=(const Digester& other);
	virtual ~Digester();
	void addRule(const Xml_string pattern, Action action);
	
	void parseFile(const Xml_string filename);
	void parseMemory(const char* data, unsigned int length);
	
	template<class Obj_Type>
		Obj_Type*  result();	
	int nrOfErrors() const;
	const Xml_string getError(int i) const;
	
// called by SAX parser:
	void beginDoc();
	void endDoc();
	void begin(Xml_string tag, Xml_attr attr);
	void end(Xml_string tag);
	void chars(Xml_string text);
	
// used by actions:
	void fail();
	void error(const Xml_string msg);
	
	template<class Obj_Type>
	Obj_Type*  top(unsigned int offset = 0);
	
	template<class Obj_Type>
	Obj_Type*  bottom(unsigned int offset = 0);
	
	template<class Obj_Type>
	void setResult(	Obj_Type* res );
	
	void pop();
	void pop(unsigned int number);
	
	template<class Obj_Type>
	void push(Obj_Type* obj);

private:
	RuleState* state;
	std::vector<void*> objects;
	void* result_;
	std::vector<Xml_string> errors;
};


template<class Obj_Type>
inline
Obj_Type*  Digester::top(unsigned int offset) 
{
#ifdef DESAXE_DEBUG
	std::cerr << "top(" << offset << ") of " << objects.size() << "\n";
#endif
	unsigned int count = objects.size();
	assert (offset < count); 
#ifdef DESAXE_DEBUG
	std::cerr << "stack-> " << static_cast<Obj_Type*>(objects[count - offset - 1]) << "\n";
#endif
	return static_cast<Obj_Type*>(objects[count - offset - 1]);
}

template<class Obj_Type>
inline
Obj_Type*  Digester::bottom(unsigned int offset) 
{ 
#ifdef DESAXE_DEBUG
	std::cerr << "bottom(" << offset << ") of " << objects.size() << "\n";
#endif
	unsigned int count = objects.size();
	assert (offset < count); 
	return static_cast<Obj_Type*>(objects[offset]);
}


template<class Obj_Type>
inline
Obj_Type*  Digester::result() 
{ 
#ifdef DESAXE_DEBUG
	std::cerr << "result-> " << static_cast<Obj_Type*>(result_) << "\n";
#endif
	return static_cast<Obj_Type*>(result_);
}


template<class Obj_Type>
inline
void Digester::setResult(Obj_Type* res) 
{ 
#ifdef DESAXE_DEBUG
	std::cerr << res << " ->result\n";
#endif
	result_ = res;
}


inline
void Digester::pop()
{
	unsigned int count = objects.size();
	assert (1 <= count);
	objects.pop_back();
}

inline
void Digester::pop(unsigned int number)
{
	unsigned int count = objects.size();
	assert (number <= count);
	objects.resize(count - number);
}


template<class Obj_Type>
inline
void Digester::push(Obj_Type* obj)
{
#ifdef DESAXE_DEBUG
	std::cerr << "stack<- " << obj << "\n";
#endif
	objects.push_back(const_cast<void*>(static_cast<const void*>(obj)));
}

} // namespace

#endif
