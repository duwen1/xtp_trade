#include "quote_spi.h"
#include <iostream>
#include <stdio.h>
using namespace std;



void MyQuoteSpi::OnError(XTPRI *error_info, bool is_last)
{
	cout << "--->>> "<< "OnRspError" << endl;
	IsErrorRspInfo(error_info);
}

MyQuoteSpi::MyQuoteSpi()
{
	my_redis = redisConnect("127.0.0.1", 6379);
}


MyQuoteSpi::~MyQuoteSpi()
{
}

void MyQuoteSpi::OnDisconnected(int reason)
{
	cout << "--->>> " << "OnDisconnected quote" << endl;
	cout << "--->>> Reason = " << reason << endl;
	//���ߺ󣬿�����������
	//�������ӳɹ�����Ҫ���������������������
}

void MyQuoteSpi::OnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last)
{
 	// cout << "OnRspSubMarketData -----" << endl;
}

void MyQuoteSpi::OnUnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last)
{
 	cout << "OnRspUnSubMarketData -----------" << endl;
}

void MyQuoteSpi::OnDepthMarketData(XTPMD * market_data, int64_t bid1_qty[], int32_t bid1_count, int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count, int32_t max_ask1_count)
{
	cout<<market_data->exchange_id<<","<<market_data->ticker<<","<< market_data->data_time<<", lastprice="<<market_data->last_price<<", ask1="<<market_data->ask[0]<<", bid1="<<market_data->bid[0]
	<<", open_price="<<market_data->open_price<<", turnover="<<market_data->turnover<<", high_price="<<market_data->high_price<<", low_price="<<market_data->low_price
	<<", upper_limit_price="<<market_data->upper_limit_price<<", lower_limit_price="<<market_data->lower_limit_price<<endl;
	string insid = market_data->ticker;
	if (int(market_data->exchange_id)==1)
	{
		string command0 = "set "+insid+"_exchange sh"; 
		(redisReply*)redisCommand(my_redis, command0.c_str()); 
	}
	else if(int(market_data->exchange_id)==2)
	{
		string command0 = "set "+insid+"_exchange sz"; 
		(redisReply*)redisCommand(my_redis, command0.c_str()); 
	}
	string command1 = "set "+insid+"_ask "+std::to_string(market_data->ask[0]); 
	(redisReply*)redisCommand(my_redis, command1.c_str()); 
	string command2 = "set "+insid+"_last "+std::to_string(market_data->last_price); 
	(redisReply*)redisCommand(my_redis, command2.c_str()); 
	string command3 = "set "+insid+"_bid "+std::to_string(market_data->bid[0]); 
	(redisReply*)redisCommand(my_redis, command3.c_str()); 
	string command4 = "set "+insid+"_open "+std::to_string(market_data->open_price); 
	(redisReply*)redisCommand(my_redis, command4.c_str()); 
	string command5 = "set "+insid+"_turnover "+std::to_string(market_data->turnover); 
	(redisReply*)redisCommand(my_redis, command5.c_str()); 
	string command6 = "set "+insid+"_high "+std::to_string(market_data->high_price); 
	(redisReply*)redisCommand(my_redis, command6.c_str()); 
	string command7 = "set "+insid+"_low "+std::to_string(market_data->low_price); 
	(redisReply*)redisCommand(my_redis, command7.c_str()); 
	string command8 = "set "+insid+"_upper_limit_price "+std::to_string(market_data->upper_limit_price); 
	(redisReply*)redisCommand(my_redis, command8.c_str()); 
	string command9 = "set "+insid+"_lower_limit_price "+std::to_string(market_data->lower_limit_price); 
	(redisReply*)redisCommand(my_redis, command9.c_str()); 
	string command10 = "set "+insid+"_datetime "+std::to_string(market_data->data_time); 
	(redisReply*)redisCommand(my_redis, command10.c_str()); 

}

void MyQuoteSpi::OnSubOrderBook(XTPST *ticker, XTPRI *error_info, bool is_last)
{

}

void MyQuoteSpi::OnUnSubOrderBook(XTPST *ticker, XTPRI *error_info, bool is_last)
{

}

void MyQuoteSpi::OnSubTickByTick(XTPST *ticker, XTPRI *error_info, bool is_last)
{

}

void MyQuoteSpi::OnUnSubTickByTick(XTPST * ticker, XTPRI * error_info, bool is_last)
{
}

void MyQuoteSpi::OnOrderBook(XTPOB *order_book)
{

}

void MyQuoteSpi::OnTickByTick(XTPTBT *tbt_data)
{

}

void MyQuoteSpi::OnQueryAllTickers(XTPQSI * ticker_info, XTPRI * error_info, bool is_last)
{
	cout << "OnQueryAllTickers -----------" << endl;
}

void MyQuoteSpi::OnQueryTickersPriceInfo(XTPTPI * ticker_info, XTPRI * error_info, bool is_last)
{
}

void MyQuoteSpi::OnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnUnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnUnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnUnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnSubscribeAllOptionMarketData(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnUnSubscribeAllOptionMarketData(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnSubscribeAllOptionOrderBook(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnUnSubscribeAllOptionOrderBook(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnSubscribeAllOptionTickByTick(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

void MyQuoteSpi::OnUnSubscribeAllOptionTickByTick(XTP_EXCHANGE_TYPE exchange_id, XTPRI * error_info)
{
}

bool MyQuoteSpi::IsErrorRspInfo(XTPRI *pRspInfo)
{
	// ���ErrorID != 0, ˵���յ��˴������Ӧ
	bool bResult = ((pRspInfo) && (pRspInfo->error_id != 0));
	if (bResult)
		cout << "--->>> ErrorID=" << pRspInfo->error_id << ", ErrorMsg=" << pRspInfo->error_msg << endl;
	return bResult;
}

