#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

template<class T>
class WriteScope;
template<class T>
class ReadScope;

// Idea: Producer and consumer have own copies of data that only they can access
template<class T>
class SPSCData
{
    friend class WriteScope<T>;
    friend class ReadScope<T>;

    enum class DataState
    {
        Invalid,
        Updated,
        Consumed
    };

    struct DataStore
    {
        T data;
        DataState state = DataState::Invalid;
    };

public:
    SPSCData()
    {
        producerData = &storage[0];
        consumerData = &storage[1];
        sharedData = &storage[2];
    }

protected:
    SPSCData<T>* acquireWrite()
    {
        bool expected = false;
        if (writing.compare_exchange_weak(expected, true))
        {
            return this;
        }

        return nullptr;
    }

    void releaseWrite()
    {
        bool expected = true;
        writing.compare_exchange_weak(expected, false);
    }

        SPSCData<T>* acquireRead()
    {
        bool expected = false;
        if (reading.compare_exchange_weak(expected, true))
        {
            return this;
        }

        return nullptr;
    }

    void releaseRead()
    {
        bool expected = true;
        reading.compare_exchange_weak(expected, false);
    }

    // used by producer to push changes to consumer
    void commit()
    {
        producerData->state = DataState::Updated;
        producerData = sharedData.exchange(producerData);
    }

    // Check and retrieve new version of data structure
    bool fetch()
    {
        consumerData = sharedData.exchange(consumerData);
        
        if (consumerData->state != DataState::Updated)
            return false;

        consumerData->state = DataState::Consumed;

        return true;
    }

private:
    DataStore storage[3];
    DataStore* producerData = nullptr;
    DataStore* consumerData = nullptr;
    std::atomic<DataStore*> sharedData;
    std::atomic_bool writing = false;
    std::atomic_bool reading = false;

};

template<class T>
class WriteScope
{
public:
    WriteScope(SPSCData<T>& data)
    {
        store = data.acquireWrite();
    }
    ~WriteScope()
    {
        if (store)
        {
            store->commit();
            store->releaseWrite();
        }
    }

    bool isValid()
    {
        return store != nullptr;
    }

    T* get() 
    { 
        return &store->producerData->data; 
    }

    T* operator ->()
    {
        return get();
    }

private:
    SPSCData<T>* store = nullptr;
};

template<class T>
class ReadScope
{
public:
    ReadScope(SPSCData<T>& data) 
    { 
        store = data.acquireRead();

        if (store && store->fetch())
            ptr = &store->consumerData->data;
    }
    ~ReadScope() 
    { 
        if (store)
            store->releaseRead();
    }

    bool isValid() const
    {
        return ptr != nullptr;
    }

    const T* get() const { return ptr; }

    const T* operator ->() const
    {
        return get();
    }

private:
    SPSCData<T>* store = nullptr;
    const T* ptr = nullptr;

};
