#include <iostream>
#include <fstream>
#include "nemodel.h"

using namespace name_entity;
using namespace std;


int fn_iInitWeiboDataFromFile(const char *dataPath, vector<Weibo> &weibo)
{
		//check path
	if(NULL == dataPath)
	{
		cerr<<"weibo data path error"<<endl;
		return 1;
	}

		//open file
	ifstream in(dataPath);
	if(!in)
		return 1;

		//parse data
	string record;
	int counter = 0;
	while(getline(in, record))
	{
		if(record == "")
			continue;
		Weibo new_doc;
		new_doc.id = counter;
		counter++;
		new_doc.source = record;
		weibo.push_back(new_doc);
	}
	in.close();
	return 0;
}

int main(int argc, char **argv)
{
	//test();
    if (argc < 2)
    {
        cout << "miss input file" << endl;
        return 1;
    }
    string confPath = "../conf/ictlap.conf";
    CNEModel model(confPath);
    vector<Weibo> text;
    string fpath = argv[1];
    fn_iInitWeiboDataFromFile(fpath.c_str(), text);
	vector<pstWeibo> corpus;
    for (int i = 0; i < text.size(); i++)
        corpus.push_back(&text[i]);

    vector<neModelRes> res;

    if (!model.BatchEntityExtract(corpus, res))
    {
        cout<<"name entity extract failed" << endl;
        return 1;
    }

    for (int i = 0; i < corpus.size(); i++)
    {
        cout<<i<<" "<<res[i].m_vOrgs.size()<<endl;
    }
	return 0;
}

