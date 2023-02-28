#include "bbt/timer/timewheel.hpp"

#define BBT_TW_LV1_Slot_MS   (__bbt_tickonce_ms__)
#define BBT_TW_LV2_Slot_MS   (BBT_TW_LV1_Slot_MS*__bbt_slot_num__)
#define BBT_TW_LV3_Slot_MS   (BBT_TW_LV2_Slot_MS*__bbt_slot_num__)


template<typename CallableType>
void TimeWheel<CallableType>::TimeWheel_Impl::TickTack()
{
    auto& current_slot = m_wheel_lv1[m_current_index_lv1];
    while(!current_slot.empty())
    {
        auto& it = current_slot.top();
        assert(it->Is_Expired());
        it->TickTack();
        current_slot.pop();
    }
    WheelLv1RotateOnce();
}

template<typename CallableType>
void TimeWheel<CallableType>::TimeWheel_Impl::WheelLv1RotateOnce()
{
    // printf("---> lv1 从动: %d <---\n",m_current_index_lv1);
    m_current_index_lv1++;
    if (m_current_index_lv1 >= __bbt_slot_num__)
    {
        m_current_index_lv1 = 0;
        WheelLv2RotateOnce();
    }
}
template<typename CallableType>
void TimeWheel<CallableType>::TimeWheel_Impl::WheelLv2RotateOnce()
{
    printf("---> lv2 从动: %d <---\n",m_current_index_lv2);
    m_current_index_lv2++; // 从动
    if (m_current_index_lv2 >= __bbt_slot_num__)
    {
        // lv2轮带动lv3推进
        m_current_index_lv2 = 0;
        WheelLv3RotateOnce();
    }

    auto begin = m_begin_timestamp + bbt::timer::milliseconds(
                                            m_current_index_lv3*BBT_TW_LV3_Slot_MS + 
                                            m_current_index_lv2*BBT_TW_LV2_Slot_MS +
                                            m_current_index_lv1*BBT_TW_LV1_Slot_MS);
    // re map    
    DoDelayQueueToWheelMap(
        m_wheel_lv2[m_current_index_lv2],
        m_wheel_lv1,
        __bbt_slot_num__,
        begin,
        BBT_TW_LV1_Slot_MS);

}

template<typename CallableType>
void TimeWheel<CallableType>::TimeWheel_Impl::WheelLv3RotateOnce()
{
    printf("---> lv3 从动: %d <---\n",m_current_index_lv3);
    m_current_index_lv3++; // 从动
    if (m_current_index_lv3 >= __bbt_slot_num__)
    {
        m_current_index_lv3=0;
        DelayQueueRotate();
    }
    auto begin = m_begin_timestamp + bbt::timer::milliseconds(
                                            m_current_index_lv3*BBT_TW_LV3_Slot_MS + 
                                            m_current_index_lv2*BBT_TW_LV2_Slot_MS +
                                            m_current_index_lv1*BBT_TW_LV1_Slot_MS);
    // re map    
    DoDelayQueueToWheelMap(
        m_wheel_lv3[m_current_index_lv3],
        m_wheel_lv2,
        __bbt_slot_num__,
        begin,
        BBT_TW_LV2_Slot_MS);
}

template<typename CallableType>
void TimeWheel<CallableType>::TimeWheel_Impl::DelayQueueRotate()
{
    assert(!m_current_index_lv1 && !m_current_index_lv2 && !m_current_index_lv3);
    // 重置
    m_begin_timestamp = m_begin_timestamp + bbt::timer::milliseconds(__bbt_max_range_of_timeout_ms__);
    m_begin_timestamp = m_begin_timestamp + bbt::timer::milliseconds(__bbt_max_range_of_timeout_ms__);

    auto begin = m_begin_timestamp + bbt::timer::milliseconds(
                                            m_current_index_lv3*BBT_TW_LV3_Slot_MS + 
                                            m_current_index_lv2*BBT_TW_LV2_Slot_MS +
                                            m_current_index_lv1*BBT_TW_LV1_Slot_MS);
    DoDelayQueueToWheelMap(
        m_delay_queue,
        m_wheel_lv3,
        __bbt_slot_num__,
        begin,
        BBT_TW_LV3_Slot_MS
    );
}

template<typename CallableType>
typename TimeWheel<CallableType>::TimeWheel_Impl::DelayQueue* 
    TimeWheel<CallableType>::TimeWheel_Impl::GetDelayQueueByTimestamp(bbt::timer::Timestamp<bbt::timer::ms> timestamp)
{
    DelayQueue* queue_ptr = nullptr;
    do{
        if (timestamp > m_end_timestamp)
        {
            queue_ptr = &m_delay_queue;
            break;
        }
        auto [success,n1,n2,n3] = GetIndexsByTimestamp(timestamp); 
        if (!success)
            return nullptr;
        // 如果需要得到所在的区间,需要判断是否在delayqueue中
        if (n3 > m_current_index_lv3)
        {
            queue_ptr = &m_wheel_lv3[n3];
            printf("lv3: %d\n",n3,timestamp.time_since_epoch().count());
            break;
        }
        else if (n3 == m_current_index_lv3)
        {
            if (n2 > m_current_index_lv2)
            {   
                queue_ptr = &m_wheel_lv2[n2];
                printf("lv2: %d\n",n2,timestamp.time_since_epoch().count());
                break;
            }
            else if(n2 == m_current_index_lv2)
            {
                if (n1 >= m_current_index_lv1)
                {
                    queue_ptr = &m_wheel_lv1[n1];
                    printf("lv1: %d\n",n1,timestamp.time_since_epoch().count());
                    break;
                }
                else
                    printf("error");
                    // break;
            }
            else
                printf("error");
                // break;
        }
        else
            printf("error");
            // break;
    }while(0);
    return queue_ptr;
}

#define IF_ZERO_OR_MINUS_ONE(expression) ( ((expression) == 0) ? 0 : (expression-1) ) 
template<typename CallableType>
std::tuple<bool,int,int,int> TimeWheel<CallableType>::TimeWheel_Impl::GetIndexsByTimestamp(bbt::timer::Timestamp<bbt::timer::ms> timestamp)
{
    bool success = false;
    timestamp += bbt::timer::milliseconds(__bbt_tickonce_ms__);
    int wheel_index[3]={0,0,0};
    do{
        if (timestamp >= m_end_timestamp)
            break;
        auto diff_ms = timestamp - m_begin_timestamp; 
        auto diff_ms_num = diff_ms.count(); // 再减去一次tick,确保tick到此slot所有事件都是超时的
        int begin_pass_ms_num = 
            m_current_index_lv1 * BBT_TW_LV1_Slot_MS +
            m_current_index_lv2 * BBT_TW_LV2_Slot_MS +
            m_current_index_lv3 * BBT_TW_LV3_Slot_MS;
        assert(begin_pass_ms_num < diff_ms_num);

        wheel_index[2] = diff_ms_num/BBT_TW_LV3_Slot_MS; diff_ms_num %= BBT_TW_LV3_Slot_MS;
        wheel_index[1] = diff_ms_num/BBT_TW_LV2_Slot_MS; diff_ms_num %= BBT_TW_LV2_Slot_MS;
        wheel_index[0] = diff_ms_num/BBT_TW_LV1_Slot_MS;
        success = true;
    }while(0);

    return std::make_tuple(success,wheel_index[0],wheel_index[1],wheel_index[2]);
}
#undef IF_ZERO_OR_MINUS_ONE 

template<typename CallableType>
void TimeWheel<CallableType>::TimeWheel_Impl::DoDelayQueueToWheelMap(
    DelayQueue& queue,
    TimeWheelMap& wheel,
    int slotnum,
    bbt::timer::Timestamp<bbt::timer::ms> begin_time,
    int slot_interval_ms)
{
    int cur_index = 0;
    while(!queue.empty())
    {
        auto task_ptr = queue.top();
        while(cur_index<slotnum)
        {   
            if (task_ptr->GetTimeOut() >= (begin_time+bbt::timer::milliseconds(slot_interval_ms*cur_index)) &&
                task_ptr->GetTimeOut() < (begin_time+bbt::timer::milliseconds(slot_interval_ms*(cur_index+1))) )
            {
                wheel[cur_index].push(task_ptr);
                break;
            }
            else
                ++cur_index;
        }
        queue.pop();
    }
}

template<typename CallableType>
bbt::timer::Timestamp<bbt::timer::ms> TimeWheel<CallableType>::TimeWheel_Impl::GetNextSlotTimestamp()
{   
    auto NextTimeOut = m_begin_timestamp + bbt::timer::milliseconds(
        m_current_index_lv1 * BBT_TW_LV1_Slot_MS +
        m_current_index_lv2 * BBT_TW_LV2_Slot_MS +
        m_current_index_lv3 * BBT_TW_LV3_Slot_MS
    );
    // NextTimeOut = NextTimeOut + bbt::timer::milliseconds(__bbt_tickonce_ms__);  
    return (NextTimeOut + bbt::timer::milliseconds(__bbt_tickonce_ms__));   // 保证slot所有事件均超时
}



#undef BBT_TW_LV1_Slot_MS
#undef BBT_TW_LV2_Slot_MS
#undef BBT_TW_LV3_Slot_MS