#include "nemodel.h"


using namespace std;
using namespace name_entity;
using namespace ICTLAP::CORE;
using namespace ICTLAP::CORE::NE;


CNEModel::CNEModel(const string &rConfPath)
{
    //FLAGS_log_dir = "../logs/nemodel/";
    //google::InitGoogleLogging("nemodel");
    m_hNEModel = NE::Init(rConfPath.c_str());
    if (m_hNEModel == NULL)
    {
        LOG(FATAL) << "NE Model init failed" << endl;
    }
    else
    {
        LOG(INFO) << "NE Model init succeed" << endl;
    }
}


CNEModel::~CNEModel()
{
    NE::Exit(m_hNEModel);
    LOG(INFO) << "NE Model exit" << endl;
}


bool CNEModel::__ParseNEResult(const string &rText, NE::NEres res, neModelRes &rRes)
{
    if (res == NULL)
    {
        LOG(WARNING) << "NEres is NULL when parse result" << endl;
        return false;
    }
    list<annoteNode> neNodes = res->labels;
    list<annoteNode>::iterator it;

    for (it = neNodes.begin(); it != neNodes.end(); ++it)
    {
        int type = it->tp;
        int nOffset = it->offset;
        int nLen = it->len;
        string sEntity = rText.substr(nOffset, nLen);
        switch (type)
        {
            case peo:
            {
                rRes.m_vPeos.push_back(sEntity);
                break;
            }
            case org:
            {
                rRes.m_vOrgs.push_back(sEntity);
                break;
            }
            case loc:
            {
                rRes.m_vLocs.push_back(sEntity);
                break;
            }
            case locC:
            {
                rRes.m_vCountries.push_back(sEntity);
                break;
            }
            case stock:
            {
                rRes.m_vStocks.push_back(sEntity);
                break;
            }
            case neTime:
            {
                rRes.m_vTimes.push_back(sEntity);
                break;
            }
        }
    }
    return true;
}


bool CNEModel::ExtractNameEntity(pstWeibo pDoc, neModelRes &rModelRes)
{
    string rText = pDoc->source;
    int nSize = rText.size();
    if (nSize == 0)
    {
        LOG(WARNING) << "text is empty when extract name entity " << endl;
        return false;
    }
    NE::NEres res = NE::Parse(m_hNEModel, rText.c_str(), nSize);
    if (res == NULL)
    {
        LOG(WARNING) << "NE Extract failed " << rText << endl;
        return false;
    }
    else
    {
        if (!__ParseNEResult(rText, res, rModelRes))
        {
            LOG(WARNING) << "Parse ne result failed " << endl;
            return false;
        }
        return true;
    }
}


bool CNEModel::BatchEntityExtract(vector<pstWeibo> &rCorpus, vector<neModelRes> &vRes)
{
    if (rCorpus.empty())
    {
        LOG(ERROR) << "BatchEntityExtract failed corpus is empty" << endl;
        return false;
    }
    vRes.clear();
    for (int i = 0; i < rCorpus.size(); i++)
    {
        neModelRes re;
        if (!ExtractNameEntity(rCorpus[i], re))
        {
            LOG(WARNING) << "Extract doc Failed" << endl;
        }
        vRes.push_back(re);
    }
    LOG(INFO) << "BatchEntityExtract succeed" << endl;
    return true;
}
