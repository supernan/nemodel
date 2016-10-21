#include <map>
#include "nemodel.h"


using namespace std;
using namespace name_entity;
using namespace ICTLAP::CORE;
using namespace ICTLAP::CORE::NE;


// get the core number of system
int GetCoreNum()
{
    int count = 0;
	count = sysconf(_SC_NPROCESSORS_CONF);
    return count;
}


void name_entity::Lock(CNEModel *hNEModel)
{
    pthread_mutex_lock(&(hNEModel->m_iModelMutex));
}


void name_entity::UnLock(CNEModel *hNEModel)
{
	pthread_mutex_unlock(&(hNEModel->m_iModelMutex));
}


void* name_entity::ExtractEntityThreadFunc(void *arg)
{
    neModelThreadParam *param = (neModelThreadParam*)arg;
    vector<pstWeibo> *pDocs = param->m_pCorpus;
    vector<neModelRes> *pRes = param->m_pRes; // must be allocate before
    CNEModel *hNEModel = param->m_pNEModel;
    NEapi API = param->m_API;
    int nStart = param->m_nStart;
    int nEnd = param->m_nEnd;

    map<int, neModelRes> mTemp;
    for (int i = nStart; i < nEnd; i++)
    {
        neModelRes res;
        if (!hNEModel->ExtractNameEntity(API, (*pDocs)[i], res))
        {
            LOG(WARNING) << "ExtractEntityThreadFunc WARNING ExtractNameEntity Failed" << endl;
            neModelRes emptyRes;
            mTemp[i] = emptyRes;
            continue;
        }
        else
            mTemp[i] = res;
    }

    Lock(hNEModel);
    for (int i = nStart; i < nEnd; i++)
    {
        if (i < 0 || i >= pRes->size())
        {
            LOG(WARNING) << "ExtractEntityThreadFunc WARNING doc id out of range " << i << endl;
            continue;
        }
        (*pRes)[i] = mTemp[i];
    }
    UnLock(hNEModel);
}



CNEModel::CNEModel(const string &rConfPath)
{
    //FLAGS_log_dir = "../logs/nemodel/";
    //google::InitGoogleLogging("nemodel");
    int nThreads = GetCoreNum();
    if (nThreads == 0)
    {
        LOG(FATAL) << "thread num is 0 Model Init Failed" << endl;
    }
    for (int i = 0; i < nThreads; i++)
    {
        NEapi hNEModel = NE::Init(rConfPath.c_str());
        if (hNEModel == NULL)
        {
            LOG(ERROR) << "NE Model init failed" << endl;
            continue;
        }
        else
        {
            LOG(INFO) << "NE Model init succeed " << i << endl;
        }
        m_vNEHandlers.push_back(hNEModel);
    }
    LOG(INFO) << "NEModels " << m_vNEHandlers.size() << endl;
}


CNEModel::~CNEModel()
{
    for (int i = 0; i < m_vNEHandlers.size(); i++)
        NE::Exit(m_vNEHandlers[i]);
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


bool CNEModel::ExtractNameEntity(NEapi pAPI, pstWeibo pDoc, neModelRes &rModelRes)
{
    string rText = pDoc->source;
    int nSize = rText.size();
    if (pAPI == NULL)
    {
        LOG(ERROR) << "NEApi is NULL" << endl;
        return false;
    }
    if (nSize == 0)
    {
        LOG(WARNING) << "text is empty when extract name entity " << endl;
        return false;
    }
    NE::NEres res = NE::Parse(pAPI, rText.c_str(), nSize);
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
    neModelRes neTempRes;
    int nSize = rCorpus.size();
    vRes.assign(nSize, neTempRes);
    int nThreads = m_vNEHandlers.size();
    if (nThreads == 0)
    {
        LOG(ERROR) << "BatchEntityExtract Error api is 0" << endl;
        return false;
    }

    int nPatchSize = nSize / nThreads;
    if (nPatchSize <= 0)
    {
        nPatchSize = nSize;
        nThreads = 1;
    }

    THPOOL::CThreadPool iNEThreadPool(nThreads);
	iNEThreadPool.fn_iInitThread();
    vector<neModelThreadParam*> vParams;
    for (int i = 0; i < nThreads; i++)
    {
        neModelThreadParam *param = new neModelThreadParam;
        if (param == NULL)
        {
            LOG(ERROR) << "Memory allocate failed in BatchEntityExtract" << endl;
            return false;
        }
        param->m_pCorpus = &rCorpus;
        param->m_pRes = &vRes;
        param->m_pNEModel = this;
        param->m_API = m_vNEHandlers[i];
        param->m_nStart = i * nPatchSize;
        if (i == nThreads - 1)
            param->m_nEnd = rCorpus.size();
        else
            param->m_nEnd = (i+1) * nPatchSize;
        vParams.push_back(param);
		iNEThreadPool.thpool_add_work(ExtractEntityThreadFunc, (void*)param);
    }

	iNEThreadPool.thpool_wait_work();
    for (int i = 0; i < vParams.size(); i++)
    {
        delete vParams[i];
        vParams[i] = NULL;
    }

    LOG(INFO) << "BatchEntityExtract succeed" << endl;
    return true;
}
