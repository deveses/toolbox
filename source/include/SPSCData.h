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
};

template<class T>
class WriteScope
{
public:
    WriteScope(SPSCData<T>* data)
    {
        store = data;
    }
    ~WriteScope()
    {
        store->commit();
    }

    T* get() { return &store->producerData->data; }

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
    ReadScope(SPSCData<T>* data) 
    { 
        if (data->fetch())
            ptr = &data->consumerData->data;
    }
    ~ReadScope() { }

    const T* get() const { return ptr; }

    const T* operator ->() const
    {
        return get();
    }

private:
    const T* ptr = nullptr;

};
