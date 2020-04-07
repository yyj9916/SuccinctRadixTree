#ifndef REGIONAL_H_
#define REGIONAL_H_
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <utility>
#include <algorithm>

using namespace std;

//全局变量
int keys_num_ = 0; //k-v对的总数
int keys_maxlength_ = 0; //keys列表中最长的字符串长度
int first_byte = 1;  //表示是否是node的第一个byte

struct Sparse {
	char SLabels;
	int SHasChild;
	int SLOUDS;
	int SValues;
	string SMulti;
	Sparse(char c = '$', int haschild = 0, int louds = 0,  int values = -1,string multi = "") :
		SLabels(c),
		SHasChild(haschild),
		SLOUDS(louds),
		SValues(values),
		SMulti(multi){}
};

class Regional
{
public:
	Regional() {};
	Regional( map<string, string>&);
	~Regional() {delete[] curr_ptr; delete[] bottom_ptr;};
	void areaListShow();
	void loudsShow();
	int getLOUDSize();
	int getNodeNum(){return node_num_;}
	void transform(map<string, string>&); //areaList->sLOUDS
public:
	vector <vector<Sparse> > sLOUDS;
private:
	vector<string> valueList;
	vector< map<int, char> > areaList; //区域化列表
	//Sparse
	int node_num_ = 0; //sLOUDS里一共有多少节点
	//assist structure，length = keys_maxlength_+1
	int* curr_ptr ; 
	int* bottom_ptr ;
};

Regional::Regional(map<string, string>& keyList) {
	int currkey = 0, last_length = 0;
	char bound_0[200];
	char bound_1[200];
	bool bound_flag = true; //true - last(0)/curr(1) , false - last(1)/curr(0)
	memset(bound_0, '$', 200); //初始化区域指示
	memset(bound_1, '$', 200); //初始化区域指示
	keys_num_ = keyList.size();

	map<string, string>::iterator it = keyList.begin();
	valueList.push_back(it->second);
	for (int i = 0; i < it->first.length(); ++i) { //初始化bound_0为第一个字符串
		bound_0[i] = it->first[i];
	}
	it++;
	last_length = it->first.length();
	
	for (; it != keyList.end(); ++it) {
		currkey++;
		valueList.push_back(it->second);
		keys_maxlength_ = (keys_maxlength_ < it->first.length()) ? it->first.length() : keys_maxlength_; // 更新最大长度

		//更新bound_curr
		if (bound_flag) {
			bound_flag = false; //取反，为下一次读取做准备
			memset(bound_1, '$', keys_maxlength_); //curr - 1
			for (int n = 0; n < it->first.length(); ++n) {
				bound_1[n] = it->first[n];
			}//标准化curr-1

			int longer = (last_length > it->first.length()) ? last_length : it->first.length();
			last_length = it->first.length(); //更新last_length
			for (int m = 0; m < longer; ++m) {
				if (m >= areaList.size()) { //添加新列
					map<int, char> temp;
					temp.insert(make_pair(currkey - 1, bound_0[m]));
					areaList.push_back(temp);
				}
				else {
					if (bound_0[m] != bound_1[m]) 
						areaList[m].insert(make_pair(currkey - 1, bound_0[m]));
				}
			}

			if (currkey == keyList.size() - 1) { //最后一个数据
				for (int m = 0; m < keys_maxlength_; ++m) {
					areaList[m].insert(make_pair(currkey, bound_1[m]));
				}
			}
			//show(bound_1, bound_flag);
		} //end if

		else {
			bound_flag = true; //取反，为下一次读取做准备
			memset(bound_0, '$', keys_maxlength_); //curr - 0
			for (int n = 0; n < it->first.length(); ++n) {
				bound_0[n] = it->first[n];
			}//标准化curr-0

			int longer = (last_length > it->first.length()) ? last_length : it->first.length();
			last_length = it->first.length(); //更新last_length
			for (int m = 0; m < longer; ++m) {
				if (m >= areaList.size()) { //添加新列
					map<int, char> temp;
					temp.insert(make_pair(currkey - 1, bound_1[m]));
					areaList.push_back(temp);
				}
				else {
					if (bound_1[m] != bound_0[m])
						areaList[m].insert(make_pair(currkey - 1, bound_1[m]));
				}
			}
			if (currkey == keyList.size() - 1) { //最后一个数据
				for (int m = 0; m < keys_maxlength_; ++m) {
					areaList[m].insert(make_pair(currkey, bound_0[m]));
				}
			}
			//show(bound_0, bound_flag);
		}//end else

	} //end for

	//为辅助结构分配内存并初始化
	curr_ptr = new int[keys_maxlength_+1];
	bottom_ptr = new int[keys_maxlength_+1];
	memset(curr_ptr, -1, keys_maxlength_+1);
	memset(bottom_ptr, 0, keys_maxlength_+1); //+1是为了处理最长的key
	//在原基础上多加一列
	map<int, char> temp;
	temp.insert(make_pair(keyList.size()-1, '$'));
	areaList.push_back(temp);
	//删除第一个key添加新列造成的错误
	for (int i = 0; i < keyList.begin()->first.length(); ++i) {
		map<int, char>::iterator it1, it2;
		it1 = it2 = areaList[i].begin();
		it2++;
		if (it1->second == it2->second) {
			areaList[i].erase(it1);
		}
	}
}

void Regional::areaListShow() {
	vector< map<int, char> >::iterator it1;
	for (it1 = areaList.begin(); it1 != areaList.end(); it1++) {
		map<int, char>::iterator it2;
		for (it2 = it1->begin(); it2 != it1->end(); it2++) {
			cout << it2->first << " - " << it2->second << " | ";
		}
		cout << endl;
	}
	cout << valueList.size()<<endl;
}

void Regional::transform(map<string, string>& keyList) {
	map<string, string>::iterator it;
	int pos,line = -1; //pos-key位置，line-当前在第几个key
	vector<char> temp;  //临时prefix存储区
	vector<int> levelpos;  //记录每个level截止的pos，在pos发生改变时返回对应level
	int level = 0,value=0,highestchange=0; //记录当前应插入第几个level,记录对应资源地址,记录bottom更新时最高被change的pos

	//初始化buttom iterator vector,指向第一位
	vector< map<int, char>::iterator > bottom_iterator;
	for (int i = 0; i <= keys_maxlength_; ++i) {
		bottom_iterator.push_back(areaList[i].begin());
	}

	for (it = keyList.begin(); it != keyList.end(); ++it) {
		line++;   //line+1
		pos = 0; //初始化labelspos
		temp.clear(); //清空temp
		string str = it->first + "$"; //str是实际处理的str
		//更新bottom_vector
		for (int m = 0; m < keys_maxlength_; ++m) {
			if (bottom_iterator[m]->first < line) bottom_iterator[m]++;
		}
		//更新bottom,记录被修改的最高位
		highestchange = keys_maxlength_;
		int origin;
		for (int n = 0; n <= keys_maxlength_; ++n) {
			origin = bottom_ptr[n];
			if (n == 0)
				bottom_ptr[n] = bottom_iterator[n]->first;
			else
				bottom_ptr[n] = min(bottom_iterator[n]->first, bottom_ptr[n - 1]);
			if (bottom_ptr[n] != origin && n < highestchange)
				highestchange = n;
		}

		//更新level
		if (levelpos.size() != 0) {
			int x = 0;
			while (levelpos[x] <= highestchange && x < levelpos.size()) x++;
			if (x < level) {
				first_byte = 0;
				while (x < level) {
					levelpos.pop_back();
					level--;
				}
			}
		}

		pos = 0;
		while (curr_ptr[pos] > line) { pos++; }//跳过公共前缀
		while (curr_ptr[pos] == line) {pos++; }
		
		//把当前pos 加入temp
		temp.push_back(str[pos]);
		pos++;

		//str终结前
		while (str[pos] != '$') {
			if (bottom_ptr[pos] == bottom_ptr[pos - 1]) { //multi节点
				temp.push_back(str[pos]);
				pos++;
			}

			else {
				//插入pos-1节点
				if (temp.size() == 1) {
					if (sLOUDS.size() <= level) {
						vector<Sparse> newlevel;
						newlevel.push_back(Sparse(temp[0], 1, first_byte, -1));
						sLOUDS.push_back(newlevel);
					}
					else {
						sLOUDS[level].push_back(Sparse(temp[0], 1, first_byte, -1));
					}
					level++;
					levelpos.push_back(pos); //标记上一层改到哪一位
					first_byte = 1;
					temp.clear(); //清空temp
				}
				else {
					//vector<char>转string
					vector<char>::iterator it;
					string s = "";
					for (it = temp.begin(); it != temp.end(); ++it) {
						s += *it;
					}
					if (sLOUDS.size() <= level) {
						vector<Sparse> newlevel;
						newlevel.push_back(Sparse(temp[0], 1, first_byte, -1, s));
						sLOUDS.push_back(newlevel);
					}
					else {
						sLOUDS[level].push_back(Sparse(temp[0],1, first_byte, -1, s));
					}
					level++;
					levelpos.push_back(pos); //标记上一层改到哪一位(+1)
					first_byte = 1;
					temp.clear(); //清空temp
				}// end else

				//插入后处理
				temp.clear(); //temp清空
				
				//将当前节点加入temp
				temp.push_back(str[pos]);
				pos++;
			} //end else
		}
		
		if (bottom_ptr[pos] < bottom_ptr[pos - 1]) { //IsPrefixKey
			if (temp.size() == 1) {
				if (sLOUDS.size() <= level) {
					vector<Sparse> newlevel;
					newlevel.push_back(Sparse(temp[0], 1, first_byte, -1)); //最后一个缺省
					sLOUDS.push_back(newlevel);
				}
				else {
					sLOUDS[level].push_back(Sparse(temp[0], 1, first_byte, -1));
				}
				level++;
				levelpos.push_back(pos); //标记上一层改到哪一位
				first_byte = 1;
				temp.clear(); //清空temp
			}
			else {
				//vector<char>转string
				vector<char>::iterator it;
				string s = "";
				for (it = temp.begin(); it != temp.end(); ++it) {
					s += *it;
				}
				if (sLOUDS.size() <= level) {
					vector<Sparse> newlevel;
					newlevel.push_back(Sparse(temp[0], 1, first_byte, -1, s));
					sLOUDS.push_back(newlevel);
				}
				else {
					sLOUDS[level].push_back(Sparse(temp[0], 1, first_byte, -1, s));
				}
				level++;
				levelpos.push_back(pos); 
				first_byte = 1;
				temp.clear(); 
			}// end else

			//再插入'$'
			if (sLOUDS.size() <= level) {
				vector<Sparse> newlevel;
				newlevel.push_back(Sparse('$', 0, first_byte, value, "$"));
				sLOUDS.push_back(newlevel);
			}
			else {
				sLOUDS[level].push_back(Sparse('$', 0, first_byte, value, "$"));//HasChild or value
			}
			value++;
			level++;
			levelpos.push_back(pos + 1);
			temp.clear(); //清空temp
		} 
		else { //Not a prefix
			//直接插入剩余char，注意修改value
			if (temp.size() == 1) {
				if (sLOUDS.size() <= level) {
					vector<Sparse> newlevel;
					newlevel.push_back(Sparse(temp[0], 0, first_byte, value));
					sLOUDS.push_back(newlevel);
				}
				else {
					sLOUDS[level].push_back(Sparse(temp[0], 0, first_byte,value));
				}
				level++;
				value++;
				levelpos.push_back(pos); //标记上一层改到哪一位
				first_byte = 1;
				temp.clear(); //清空temp
			}
			else {
				//vector<char>转string
				vector<char>::iterator it;
				string s = "";
				for (it = temp.begin(); it != temp.end(); ++it) {
					s += *it;
				}
				if (sLOUDS.size() <= level) {
					vector<Sparse> newlevel;
					newlevel.push_back(Sparse(temp[0], 0, first_byte, value, s));
					sLOUDS.push_back(newlevel);
				}
				else {
					sLOUDS[level].push_back(Sparse(temp[0], 0, first_byte, value, s));
				}
				value++;
				level++;
				levelpos.push_back(pos); //标记上一层改到哪一位(+1)
				first_byte = 1;
				temp.clear(); //清空temp
			}// end else
		}//end else
		for (int p = 0; p < str.length(); ++p) {
			curr_ptr[p] = bottom_ptr[p];
		}
	} //end for
	//更新节点数
	
	node_num_ = 0;
	for (int t = 0; t < sLOUDS.size(); ++t) {
		node_num_ += sLOUDS[t].size();
	}
}

void Regional::loudsShow() {
	vector<vector<Sparse>>::iterator it1;
	for (it1 = sLOUDS.begin(); it1 != sLOUDS.end(); ++it1) {
		for (int i = 0; i < (*it1).size(); ++i) {
			cout << (*it1)[i].SLabels << "/";
			cout << (*it1)[i].SHasChild << "/";
			cout << (*it1)[i].SLOUDS<< "/";
			cout << (*it1)[i].SValues << "/";
			cout << (*it1)[i].SMulti << " |";
		}
		cout << endl;
	}
}

int Regional::getLOUDSize() {
	return node_num_;
}
#endif
