#ifndef  __THREAD_POOL_H_
#define  __THREAD_POOL_H_

#include <string>
#include <map>
#include <pthread.h>
#include <service_util.h>

/** 最多线程数 */
#define MAX_THREAD_NUM  1024


/**
 * @brief 线程接口
 */
class Thread
{
    public:
        virtual ~Thread(){};
        /**
         * @brief
         */
        virtual std::string getId() = 0;
        /**
         * @brief 运行
         */
        virtual void run() = 0;
        /**
         * @brief 停止
         */
        virtual void stop() = 0;
        /**
         * @brief 线程本身没有工作需要处理
         */
        virtual bool over() = 0;
};

/**
 * @brief 线程运行中止回调函数
 */
typedef void (*ThreadLeaveCallback)(Thread *, void *userData);

/**
 * @brief 线程托管者，负责空闲时sleep，有任务时执行任务
 */
class  ThreadHolder
{
    private:
        /** 工作线程 */
        Thread * m_thread;
        /** 中断工具 */
        ServiceUtil m_util;
        /** 安全操作锁 */
        pthread_mutex_t m_lock;
        /** cb */
        ThreadLeaveCallback m_callBack;
        /** cb param */
        void *m_userData;
    public:
        /**
         * @brief
         */
        ThreadHolder(ThreadLeaveCallback cb=NULL, void *userData=NULL);
        /**
         * @brief
         */
        ~ThreadHolder();
        /**
         * @brief 运行函数
         */
        void run();
        /**
         * @brief 停止工作
         */
        void stop();
        /**
         * @brief 是否空闲
         */
        bool isFree();
        /**
         * @brief 设置要托管的线程，如果已经有正在托管的，则返回失败
         * @retval: 0: succeed
         *         -1: failed
         */
        int setThread(Thread *thread);
        /**
         * @brief 删除实例
         */
        int rmThread(Thread *thread);
        /**
         * @brief
         */
        Thread* getThread();
};

/**
 * @brief 线程池
 */
class ThreadPool
{
    private:
        /** ing */
        bool m_running;
        /** 线程数 */
        uint32_t m_threadNum;
        /** Maximum */
        uint32_t m_maxThreadNum;
        /** 线程 */
        std::map<pthread_t, ThreadHolder*> m_threads;
        /** 线程安全 */
        pthread_mutex_t m_lock;
        /** 线程退出回调函数 */
        ThreadLeaveCallback m_threadLeave;
        /** 线程退出回调函数参数 */
        void *m_userData;
    private:
        /**
         * @brief 
         */
        int increaseThread();
    public:
        /**
         * @brief
         */
        void setThreadLeaveCallback(ThreadLeaveCallback cb, void *userData);
        /**
         * @brief
         */
        ThreadPool();
        /**
         * @brief 析构
         */
        ~ThreadPool();
        /**
         * @brief 增加线程
         * @retval: 0:成功
         *         -1:没有空闲线程可以使用
         */
        int addThread(Thread *thread);
        /**
         * @brief 清除线程
         */
        int rmThread(Thread *thread);
        /**
         * @brief 获取全部线程
         */
        std::vector<Thread*> getThreads();
        /**
         * @brief 启动
         */
        int start(uint32_t initThreadNum, uint32_t maxThreadNum);
        /**
         * @brief
         */
        int stop();
};


#endif  //__THREAD_POOL_H_
