/*
 * similar.cpp
 * Find the most similar query
 * Date:2015-05-28 10:46:30
 *
 */
#ifndef SIMILAR_H
#define SIMILAR_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include <boost/unordered_map.hpp>
#include <util/hashFunction.h>
#include "knlp/horse_tokenize.h"

using namespace std;
bool Is_subset(vector<unsigned int> v1,vector<unsigned int> v2);

struct QueryData
{
 string text;
 float score;
 //std::size_t qtf; //terms number in query
 vector<uint32_t> tid; //terms id 
};

class Similar
{
  private:
     static const string tok_pth;
     static const string qd;
     static const string td;

     ilplib::knlp::HorseTokenize* tok_;

     boost::unordered_map<unsigned int,vector<unsigned int> > Terms_Qid;
     boost::unordered_map<unsigned int,QueryData> inFile;

  public:
      Similar();
      ~Similar();
      void init();
      void GetSim(string& q,string& res);
};

const  string Similar::tok_pth = "../dict";
const  string Similar::qd = "../dict/queryDat.v";
const  string Similar::td = "../dict/termId.v";
Similar::Similar()
{
  tok_ = new ilplib::knlp::HorseTokenize(tok_pth); 
}

void Similar::init()
{
 ifstream f_terms;
 ifstream f_inver;

 f_inver.open(qd.c_str());
 if(!f_inver.is_open())
     std::cerr << "Open  query data file error!" << std::endl;

 f_terms.open(td.c_str());
 if(!f_terms.is_open())
     std::cerr << "Open terms id file error!" << std::endl;

 //----process query data---//
 string s;
 string stmp;
 string tscore;

 size_t pos1 = -1;
 unsigned int id = 0;
 QueryData dat;
 int count = 0;
 vector<uint32_t> tp;

 while(getline(f_inver,s))
 {
     dat.tid.clear();
     tp.clear();
     //get query id
       pos1 = s.find('\t');
       if(pos1 != string::npos)
       {
           stmp = s.substr(0,pos1);
           s.assign(s,pos1+1,s.length()-1);
       }
       //get query text
       pos1 = s.find('\t');
       if(pos1 != string::npos)
       {
           dat.text = s.substr(0,pos1);
           s.assign(s,pos1+1,s.length()-1);
       }
       //get query score
       pos1 = s.find('\t');
       if(pos1 != string::npos)
       {
           tscore = s.substr(0,pos1);
           s.assign(s,pos1+1,s.length()-1);
           dat.score = atof(tscore.c_str());
       }
       //get query terms id
       count = std::count(s.begin(),s.end(),'\t');
       while(count--)
       {
           pos1 = s.find('\t');
           if(pos1 != string::npos)
           {
               tscore = s.substr(0,pos1);
               tp.push_back(atoi(tscore.c_str()));
               s.assign(s,pos1+1,s.length()-1);
           }
       }
       tp.push_back(atoi(s.c_str()));
       dat.tid = tp;
       inFile.insert(make_pair(atoi(stmp.c_str()),dat));
      // getline(f_inver,s);
 }
 //----process terms queryid---//
 getline(f_terms,s);
 
 uint32_t tid = 0;
 vector<unsigned int> tt;
 while(!f_terms.eof())
 {
     tt.clear();
     count = std::count(s.begin(),s.end(),'\t');
     //
     pos1 = s.find('\t');
     if(pos1 != string::npos)
     {
         stmp = s.substr(0,pos1);
         tid = atoi(stmp.c_str());
         s.assign(s,pos1+1,s.length()-1);
     }
     count -=1;
     while(count--)
     {
     pos1 = s.find('\t');
     if(pos1 != string::npos)
     {
         stmp= s.substr(0,pos1);
         tt.push_back(atoi(stmp.c_str()));
         s.assign(s,pos1+1,s.length()-1);
     }
     }
     tt.push_back(atoi(s.c_str()));
     Terms_Qid.insert(make_pair(tid,tt));
     getline(f_terms,s);
 }
 
 f_inver.close();
 f_terms.close();
 boost::unordered_map<unsigned int,vector<unsigned int> >::iterator it;
/* for(it = Terms_Qid.begin();it != Terms_Qid.end();++it)
 {
     cout << it->first ;
     for(unsigned int j=0;j< it->second.size();++j)
         cout << "\t"<<it->second[j] << endl;
 }*/
}

Similar::~Similar()
{
  delete tok_;
}

void Similar::GetSim(string& q,string& res)
{
 vector<pair<string,float> > r;
 vector<unsigned int> qid;
 vector<unsigned int> termid;
 vector<unsigned int> qtermid;

 boost::unordered_map<string,vector<unsigned int> > terms;
 boost::unordered_map<unsigned int,std::size_t> qidf; //query id ,qtf

 boost::unordered_map<string,vector<unsigned int> >::iterator it_tid;
 boost::unordered_map<string,vector<unsigned int> >::iterator qtf; //query terms frequency
 boost::unordered_map<unsigned int,std::size_t>::iterator it_qtf;
 boost::unordered_map<unsigned int,vector<unsigned int> >::iterator it_qid;

 r.clear();
 qid.clear();
 qidf.clear();
 termid.clear();
 qtermid.clear();

 float biggest = 0.0;
 string big_len = "";
 cout << "Input query:" << q << endl;
 try
 {
     tok_->tokenize(q,r);
 }
 catch(...)
 {
     r.clear();
 }
 //dedup 
 cout << "input query token terms:" << endl;
 for(unsigned int i = 0;i < r.size();++i)
 {
     it_tid = terms.find(r[i].first);
     if(it_tid != terms.end())
         continue;
     else
     {
		 if(big_len.length() < r[i].first.length())
			 big_len = r[i].first;
         std::cout << "," << r[i].first;
         terms.insert(make_pair(r[i].first,qid));
     }
 }
 
 //find candidate query id for every terms
 uint32_t Id = 0;
 for(it_tid = terms.begin();it_tid !=  terms.end();++it_tid)
{
 Id =  izenelib::util::izene_hashing(it_tid->first);
 termid.push_back(Id);
 //cout << "term id:" << Id << endl;

 it_qid = Terms_Qid.find(Id);
 if(it_qid != Terms_Qid.end())
 {
     //get query id
     for(unsigned int i = 0; i < it_qid->second.size();++i)
     {  it_tid->second.push_back(it_qid->second[i]);
       //  cout << "terms:"<< it_tid->first << "\tterms id:" << it_qid->first <<  "\tquery id:" << it_qid->second[i] << endl;
       //caculate qdf
      /* for(qtf = terms.begin();qtf != terms.end();++qtf)
       {
           unsigned int cn =0;
           int pos = inFile[it_tid->second[i]].text.find(it_tid->first);
           if(pos != string::npos)
           {
               cn +=1;
               qidf.insert(make_pair(it_qid->second[i],cn)); //store query id and qdf
           }
       }*/
     }
 }
}

//find the biggest score
bool subset_flag = false;
string res2;
float biggest2 = 0.0;
for(it_tid = terms.begin(); it_tid != terms.end();++it_tid)
{
  for(unsigned int j = 0;j< it_tid->second.size();++j)
  {
     //pruning
     // it_qdf = qidf.find(it_tid->second[j]);
     // if(it_qdf != qidf.end())
     // if(terms.size() > inFile[it_tid->second[j]].tid.size() /*&& it_qdf->second > 3 */)
      // sort(termid.begin(),termid.end());
       qtermid = inFile[it_tid->second[j]].tid;
       //cout << "can size:" << qtermid.size() << "\t terms size:"<< termid.size()<< endl;
     // if(includes(termid.begin(),termid.end(),qtermid.begin(),qtermid.end()))
       if(Is_subset(termid,qtermid))
      {
          subset_flag = true;
      if(biggest < ((float)inFile[it_tid->second[j]].score * qtermid.size()))
      {
          biggest = (float)inFile[it_tid->second[j]].score * qtermid.size(); //score
          res = inFile[it_tid->second[j]].text; //query
          cout << "\nbiggest score:" << biggest << ",candicate query id:" << it_tid->second[j] 
               << ",query score:" << (float)inFile[it_tid->second[j]].score * qtermid.size() << endl;
         // cout << inFile[it_tid->second[j]].text << endl;
      }
      }
       else
       {
           if(biggest2 < ((float)inFile[it_tid->second[j]].score * qtermid.size()))
           {
               
               biggest2 = (float)inFile[it_tid->second[j]].score * qtermid.size(); //score
               res2 = inFile[it_tid->second[j]].text; //query
               cout << "\nbiggest score:" << biggest2 << ",candicate query id:" << it_tid->second[j] 
                    << ",query score:" << (float)inFile[it_tid->second[j]].score * qtermid.size() << endl;
           }
       }
  }
}
string ss = "";
float b_score = -0.01;
if(subset_flag)
{
  ss = res;
  b_score = biggest;
}
  else
{
  ss = res2;
  b_score = biggest2;
}
if(ss.length() < 4 && b_score != 0)
	ss = big_len;
res = "{\"Recomm\":\"";
if(ss == q)
	ss = "";
res += ss;
res += "\"}";
cout << "The most similar query:" << res << "\t score:" << b_score << endl; 

}

bool Is_subset(vector<unsigned int> v1,vector<unsigned int> v2)
{
  boost::unordered_map<unsigned int,unsigned int> sets;
  boost::unordered_map<unsigned int,unsigned int>::iterator it;

  sets.clear();
  for(int i = 0;i < v1.size();++i)
      sets.insert(make_pair(v1[i],1));

  for(int j = 0;j < v2.size();++j)
  {
      it = sets.find(v2[j]);
      if(it != sets.end())
          continue;
      else
          return false;
  }

  return true;
}

#endif //similar.h

