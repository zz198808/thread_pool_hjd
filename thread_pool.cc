#include "thread_pool.h"

using namespace std;
using namespace beehive;

ThreadHolder::ThreadHolder(ThreadLeaveCallback callBack, void *userData)
    :m_callBack(callBack),
    m_userData(userData)
{
    m_thread = NULL;
    pthread_mutex_init(&m_lock, NULL);
    m_util.start();
}
ThreadHolder::~ThreadHolder()
{
    pthread_mutex_destroy(&m_lock);
};
static void* threadpoolFunc(void *arg)
{
    ThreadHolder *tool = (ThreadHolder*)arg;
    tool->run();
    return NULL;
}
void ThreadHolder::run()
{
    while(m_util.isRunning()){
        //检查是否需要运行
        Thread *thread = NULL;
        ScopedLock lock(&m_lock);
        if(NULL == m_thread || m_thread->over()){
            thread = NULL;
        }
        else{
            thread = m_thread;
        }
        lock.free();
        //运行/wait
        if(NULL == thread){
            //有可能在释放锁后Ｕ饫镏氨恢飨叱躺柚昧诵氯挝瘢佣薹ǖ鞫?            //因此这里用1s来修复这个问题
            m_util.stopable_sleep(1*1000);
        }
        else{
            thread->run();
            if(NULL != m_callBack){
                m_callBack(thread, m_userData);
            }
        }
    }
};
void ThreadHolder::stop()
{
    m_util.stop();
    ScopedLock lock(&m_lock);
    if(NULL != m_thread && !m_thread->over()){
        m_thread->stop();
    }
}
bool ThreadHolder::isFree()
{
    return NULL == m_thread;
}
/**
 * @brief 
 */
int ThreadHolder::setThread(Thread *thread)
{
    ScopedLock lock(&m_lock);
    if(NULL == m_thread){
        m_thread = thread;
        m_util.wakeup();
        return 0;
    }
    else{
        return -1;
    }
};
int ThreadHolder::rmThread(Thread *thread)
{
    ScopedLock lock(&m_lock);
    if(NULL == m_thread){ //空闲Holder
        return -1;
    }
    else if(thread == m_thread){//正确Holder
        if(!thread->over()){//如果正在运行，则中止之
            thread->stop();
        }
        m_thread = NULL;
        return 0;
    }
    else{//不空闲且不匹配的Holder
        return -1;
    }

};
Thread* ThreadHolder::getThread()
{
    return m_thread;
}
//////////////////////////////ThreadPool/
//
ThreadPool::ThreadPool()
{
    m_running = false;
    m_threadLeave = NULL;
    m_userData = NULL;
    m_maxThreadNum = 0;
    pthread_mutex_init(&m_lock, NULL);
}

ThreadPool::~ThreadPool()
{
    if(m_running){
        stop();
    }
    pthread_mutex_destroy(&m_lock);
};
void ThreadPool::setThreadLeaveCallback(ThreadLeaveCallback cb, void *userData)
{
    m_threadLeave = cb;
    m_userData = userData;
};
int ThreadPool::increaseThread()
{
    pthread_t pid;
    ThreadHolder *holder = new ThreadHolder(m_threadLeave, m_userData);
    int ret = pthread_create(&pid, NULL, threadpoolFunc, holder);
    if(0 != ret){
        delete holder;
        return -1;
    }
    m_threads[pid] = holder;
    return 0;
};
int ThreadPool::start(uint32_t threadNum, uint32_t maxThreadNum)
{
    if(m_running){
        bh_logwarn("ThreadPool start while started");
        return -1;
    }
    m_maxThreadNum = maxThreadNum;
    m_threadNum = threadNum;
    for(uint32_t i=0; i<m_threadNum; i++){
        int ret = increaseThread();
        if(0 != ret){
            bh_logerr("ThreadPool start thread fail");
            stop();
        }
    }
    m_running = true;
    bh_logmsg("ThreadPool start succeed threadNum=%u", m_threadNum);
    return 0;
};

int ThreadPool::stop()
{
    if(!m_running){
        bh_logwarn("ThreadPool stop while stoped");
        return -1;
    }
    m_running = false;
    ScopedLock lock(&m_lock);
    map<pthread_t, ThreadHolder*> temp_threads = m_threads; 
    for(map<pthread_t, ThreadHolder*>::iterator it=m_threads.begin();
            it!=m_threads.end(); it++){
        //停止线程
        it->second->stop();
    }
    m_threads.clear();
    lock.free();

    for(map<pthread_t, ThreadHolder*>::iterator it=temp_threads.begin();
            it!=temp_threads.end(); it++){
        pthread_join(it->first, NULL);
        delete it->second;
    }
    bh_logmsg("ThreadPool stop succeed");
    return 0;
}

int ThreadPool::addThread(Thread *thread)
{
    ScopedLock lock(&m_lock);
    //寻找空位
    for(map<pthread_t, ThreadHolder*>::iterator it=m_threads.begin();
            it!=m_threads.end(); it++){
        if(it->second->isFree()){
            if(0 == it->second->setThread(thread)){
                bh_logwarn("ThreadPool addThread succeed id=%s", thread->getId().c_str());
                return 0;
            }
        }
    }
    //是否递增
    if(m_threadNum < m_maxThreadNum){
        int ret = increaseThread();
        if(0 == ret){
            bh_logwarn("ThreadPool addThreadInc succeed id=%s", thread->getId().c_str());
            return 0;
        }
        m_threadNum ++;
        bh_logerr("ThreadPool increate thread failed");
    }
    bh_logwarn("ThreadPool addThread fail, id=%s", thread->getId().c_str());
    return -1;
}

int ThreadPool::rmThread(Thread *thread)
{
    ScopedLock lock(&m_lock);
    for(map<pthread_t, ThreadHolder*>::iterator it=m_threads.begin();
            it!=m_threads.end(); it++){
        if(0 == it->second->rmThread(thread)){
            bh_logmsg("ThreadPool rmThread succeed id=%s", thread->getId().c_str());
            return 0;
        }
    }
    bh_logmsg("ThreadPool rmThread failed id=%s", thread->getId().c_str());
    return -1;
};

vector<Thread*> ThreadPool::getThreads()
{
    vector<Thread*> result;
    ScopedLock lock(&m_lock);
    for(map<pthread_t, ThreadHolder*>::iterator it=m_threads.begin();
            it!=m_threads.end(); it++){
        if(!it->second->isFree()){
            result.push_back(it->second->getThread());
        }
    }
    return result;
};

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
