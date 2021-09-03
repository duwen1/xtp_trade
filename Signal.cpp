#include "Signal.h"
using namespace std;

#define LONGTIME 8000

Signal* Signal::pThis = nullptr;

extern XTP::API::TraderApi* pUserApi;

extern bool is_connected_;
extern std::string trade_server_ip;
extern int trade_server_port;
extern uint64_t session_id_;
extern std::map<uint64_t,uint64_t> map_session;
extern uint64_t* session_arrary;

int* get_local_time()
{
    time_t timer;
	timer = time(nullptr);
	struct tm* tblock = localtime(&timer);
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec;
    int * results= new int[4];
    results[0] = tblock->tm_hour;
    results[1] = tblock->tm_min;
    results[2] = tblock->tm_sec;
    results[3] =  milli/1000;
    return results; 
}

int get_contract_num(redisContext* my_redis, string contract)
{
    string check_open_exists = "EXISTS a1_" + contract+ "_sellable_qty";
	redisReply* r_open = (redisReply*)redisCommand(my_redis, check_open_exists.c_str());
    int num=0;
	if (r_open->integer != 0)
    {
        string contract_check = "get a1_" +  contract+ "_sellable_qty";
        redisReply* rp = (redisReply*)redisCommand(my_redis, contract_check.c_str());
        num = atoi(rp->str);
    }
   return num;
}

float get_contract_last_price(redisContext* my_redis, string contract)
{
    string check_open_exists = "EXISTS " + contract+ "_last";
	redisReply* r_open = (redisReply*)redisCommand(my_redis, check_open_exists.c_str());
    float num=0;
	if (r_open->integer != 0)
    {
        string contract_check = "get " + contract +"_last";
        redisReply* rp = (redisReply*)redisCommand(my_redis, contract_check.c_str());
        num = atof(rp->str);
    }
   return num;
}


Signal::Signal()
{
    pThis = this;
    my_redis = redisConnect("127.0.0.1", 6379);
    if (my_redis->err)
    {
        redisFree(my_redis);
        printf("Connect to redisServer faile\n");
        return;
    }
    printf("Connect to redisServer Success\n");

}
void Signal::over()
{

}

void Signal::check_hold_position()
{
    int result = pUserApi->QueryPosition(NULL,session_arrary[0], 1001, XTP_MKT_INIT); 
    cout<<"query sz position result:"<<result<<endl;
    // int result2 = pUserApi->QueryPosition(NULL,session_arrary[0], 1002, XTP_MKT_SH_A); 
    // cout<<"query sh position result:"<<result2<<endl;
    //  XTP_MKT_SZ_A = 1,///<深圳A股
    // XTP_MKT_SH_A, 
}

void Signal::start_buy(string exchangeid, string instrument, double price, int num, string order_type)
{
    XTPOrderInsertInfo* orderList = new XTPOrderInsertInfo[1];
	memset(orderList, 0, sizeof(XTPOrderInsertInfo)*1);
    orderList[0].order_client_id = 0;
    std::string instrumentid =instrument;
    strcpy(orderList[0].ticker, instrumentid.c_str());
    if (exchangeid=="SH"){
        orderList[0].market = (XTP_MARKET_TYPE)XTP_MKT_SH_A;
    }
    else{
        orderList[0].market = (XTP_MARKET_TYPE)XTP_MKT_SZ_A;
    }
    orderList[0].price = price;
    orderList[0].quantity = num;
    orderList[0].side = (XTP_SIDE_TYPE)XTP_SIDE_BUY;
    orderList[0].price_type = (XTP_PRICE_TYPE)XTP_PRICE_BEST5_OR_CANCEL;
    orderList[0].business_type = (XTP_BUSINESS_TYPE)0; 
    if (session_arrary[0] == 0)
    {
        return;
    }
    int64_t xtp_id = pUserApi->InsertOrder(&(orderList[0]), session_arrary[0]);
    cout<<"start to buy "<<instrument<<", num="<<num<< ", xtpid="<<xtp_id<<endl;
}

void Signal::place_order(string exchangeid, string instrument, double price, int num, XTP_SIDE_TYPE side)
{
    XTPOrderInsertInfo* orderList = new XTPOrderInsertInfo[1];
	memset(orderList, 0, sizeof(XTPOrderInsertInfo)*1);
    orderList[0].order_client_id = 0;
    std::string instrumentid =instrument;
    strcpy(orderList[0].ticker, instrumentid.c_str());
    if (exchangeid=="SH"){
        orderList[0].market = (XTP_MARKET_TYPE)XTP_MKT_SH_A;
    }
    else{
        orderList[0].market = (XTP_MARKET_TYPE)XTP_MKT_SZ_A;
    }
    orderList[0].price = price;
    orderList[0].quantity = num;
    orderList[0].side = (XTP_SIDE_TYPE)side;
    orderList[0].price_type = (XTP_PRICE_TYPE)XTP_PRICE_BEST5_OR_CANCEL;
    orderList[0].business_type = (XTP_BUSINESS_TYPE)0; 
    if (session_arrary[0] == 0)
    {
        return;
    }
    int64_t xtp_id = pUserApi->InsertOrder(&(orderList[0]), session_arrary[0]);
    cout<<"start to sell "<<instrument<<", num="<<num<< ", xtpid="<<xtp_id<<endl;
}

void Signal::start_sell(string exchangeid, string instrument, double price, int num, string order_type)
{
    float last_price =  get_contract_last_price(my_redis,instrument);
    float need_sell_money = last_price*num;
    int need_sell_num = num;
    int total_num = get_contract_num(my_redis, instrument);
    cout<<"********************************"<<endl;
    cout<<instrument<<", exchange="<<exchangeid<<", last price ="<<last_price<<", total_num="<<total_num<<", need sell num="<<need_sell_num<<", need sell money="<<need_sell_money<<endl;
    int left_num = num;
 
    if(need_sell_money>1000000)
    {
        while(need_sell_money>1000000)
        {
            int try_times = int(need_sell_money/1000000);
            cout<<"需要发送"<<try_times<<"次循环卖单，每次卖单金额不超过100万元"<<endl;
            for(int i=0;i<try_times;i++){
                float last_price =  get_contract_last_price(my_redis,instrument);
                int sell_num = int(1000000/last_price/100)*100;
                cout<<"********************************"<<endl;
                cout<<"第"<<i+1<<"次平仓单，平仓量="<<sell_num<<", 剩余待平仓次数="<<try_times-i+1<<endl;
                place_order(exchangeid,instrument, last_price, sell_num, XTP_SIDE_SELL);
                sleep(1);
            }
            sleep(5);
            check_hold_position();
            sleep(1);
            float last_price =  get_contract_last_price(my_redis,instrument);
            left_num = get_contract_num(my_redis, instrument);
            need_sell_num = need_sell_num-(total_num-left_num);
            need_sell_money = last_price*need_sell_num;  
            cout<<"剩余待平仓量="<<need_sell_num<<", 待平仓金额="<<need_sell_money<<endl;
            cout<<"----------------"<<endl;
            if(need_sell_num<100) break;
        }
        if(need_sell_num>0)
        {
            float last_price =  get_contract_last_price(my_redis,instrument);
            cout<<"最后一笔卖单, 平仓量="<<need_sell_num<<endl;
            place_order(exchangeid,instrument, last_price, need_sell_num, XTP_SIDE_SELL);
            cout<<"********************************"<<endl;
        }
    }
    else{
        float last_price =  get_contract_last_price(my_redis,instrument);
        place_order(exchangeid, instrument, last_price, left_num, XTP_SIDE_SELL);
        cout<<"********************************"<<endl;
    }

}


void Signal::start_trading(string contract, int volume, double min_change, string side, string exchange)
{
  
   if(side=="open_long"){
        string main_check = "get " + contract + "_ask";
        redisReply* main_data = (redisReply*)redisCommand(my_redis, main_check.c_str());
        double ask_price1 = atof(main_data->str);
        cout<<"$$$$$$$$$$$$$$$"<<endl;
        cout<<contract <<", need open long "<<", price ="<<ask_price1 + 3*min_change<<", volume="<<volume<<endl;
        cout<<"$$$$$$$$$$$$$$$"<<endl;
    
   }
   else if(side=="open_short"){
        string main_check = "get " + contract + "_bid";
        redisReply* main_data = (redisReply*)redisCommand(my_redis, main_check.c_str());
        double bid_price1 = atof(main_data->str);
        cout<<"$$$$$$$$$$$$$$$"<<endl;
        cout<<contract <<", need open short "<<", price ="<<bid_price1 - 3*min_change<<", volume="<<volume<<endl;
        cout<<"$$$$$$$$$$$$$$$"<<endl;
        
   }
   else if(side=="close_long"){
        string main_check = "get " + contract + "_bid";
        redisReply* main_data = (redisReply*)redisCommand(my_redis, main_check.c_str());
        double bid_price1 = atof(main_data->str);
        cout<<"$$$$$$$$$$$$$$$"<<endl;
        cout<<contract <<", need close long "<<", price ="<<bid_price1 - 3*min_change<<", volume="<<volume<<endl;
        cout<<"$$$$$$$$$$$$$$$"<<endl;
     
   }
   else if(side=="close_short"){
        string main_check = "get " + contract + "_ask";
        redisReply* main_data = (redisReply*)redisCommand(my_redis, main_check.c_str());
        double ask_price1 = atof(main_data->str);
        cout<<"$$$$$$$$$$$$$$$"<<endl;
        cout<<contract <<", need close short "<<", price ="<<ask_price1 + 3*min_change<<", volume="<<volume<<endl;
        cout<<"$$$$$$$$$$$$$$$"<<endl;
   }
}


void Signal::load_last_data(string filename)
{
    check_hold_position();
    sleep(1);
    ifstream infile;
    string symbol;
    infile.open(filename, ios::in);
    if (!infile.is_open())
        cout << "Open file failure" << endl;
    int i=0;
    while (!infile.eof())        
    {
        infile >> symbol;
        if(i==0)
        {
            last_symbols[0] = symbol;
        }
        else if(i==1)
        {
            last_symbols[1] = symbol;
        }
        i++;
    }
    infile.close();
    int last_hold_num1 = get_contract_num(my_redis, last_symbols[0]);
    int last_hold_num2 = get_contract_num(my_redis, last_symbols[1]);

    cout<<"----------------------------------------------------------------------------------------------"<<endl;
    cout<<"上一日信号如下："<<endl;
    cout<<"******************"<<endl;
    cout<<"合约1："<<last_symbols[0]<<endl;
    cout<<"今日持有量（可卖量）:"<<last_hold_num1<<endl;
    cout<<"******************"<<endl;
    cout<<"合约2："<<last_symbols[1]<<endl;
    cout<<"今日持有量（可卖量）:"<<last_hold_num2<<endl; 

    string init_num1 = "set a1_" + last_symbols[0] + "_hold_num " + std::to_string(last_hold_num1);
    (redisReply*)redisCommand(my_redis, init_num1.c_str());
    string init_num2 = "set a1_" + last_symbols[1] + "_hold_num " + std::to_string(last_hold_num2);
    (redisReply*)redisCommand(my_redis, init_num2.c_str());
}

void Signal::load_signal_data(string filename)
{

    // start_sell("SH","600000",10, 100000, "market");
    
    ifstream infile;
    string symbol;
    infile.open(filename, ios::in);
    if (!infile.is_open())
        cout << "Open file failure" << endl;
    int i=0;
    while (!infile.eof())        
    {
        infile >> symbol;
        if(i==0)
        {
            today_symbols[0] = symbol;
        }
        else if(i==1)
        {
            today_symbols[1] = symbol;
        }
        i++;
    }
    infile.close();

    float contact_price1 = get_contract_last_price(my_redis, today_symbols[0]);
    float contact_price2 = get_contract_last_price(my_redis, today_symbols[1]);

    if (contact_price1<1 or contact_price2<1)
    {
        cout<<"行情服务获取失败， 当天不交易"<<endl;
        return;
    }
    int contract_hold_num1 = get_contract_num(my_redis, today_symbols[0]);
    int contract_hold_num2 = get_contract_num(my_redis, today_symbols[1]);

    int need_hold_num1 = int(invest_money *0.5 / contact_price1 /100 )*100;
    int need_hold_num2 = int(invest_money *0.5 / contact_price2 /100 )*100;
    double need_hold_money1= contact_price1*need_hold_num1;
    double need_hold_money2= contact_price2*need_hold_num2;
    double need_hold_money_total = need_hold_money1+need_hold_money2;
    cout<<"need_hold_num1="<<need_hold_num1<<", need_hold_num2="<<need_hold_num2<<endl;
    cout<<"need_hold_money1="<<need_hold_money1<<", need_hold_money2="<<need_hold_money2<<", need_hold_money_total="<<need_hold_money_total<<endl;

    cout<<"----------------------------------------------------------------------------------------------"<<endl;
    cout<<"今日平仓信号如下："<<endl;
    int last_hold_num1 = get_contract_num(my_redis, last_symbols[0]);
    int last_hold_num2 = get_contract_num(my_redis, last_symbols[1]);
  
    int need_close_num1 = 0;
    int need_close_num2 = 0;

    if(last_symbols[0] ==  today_symbols[0]){
        need_close_num1 = max(last_hold_num1-need_hold_num1, 0);
    }
    else if(last_symbols[0] ==  today_symbols[1]){
        need_close_num1 = max(last_hold_num1-need_hold_num2, 0);
    }
    else{
        need_close_num1 = last_hold_num1;
    }

    if(last_symbols[1] ==  today_symbols[0]){
        need_close_num2 = max(last_hold_num2-need_hold_num1, 0);
    }
    else if(last_symbols[1] ==  today_symbols[1]){
        need_close_num2 = max(last_hold_num2-need_hold_num2, 0);
    }
    else{
        need_close_num2 = last_hold_num2;
    }


    bool need_close = false;
    if(need_close_num1>0){
        cout<<"******************"<<endl;
        cout<<"卖出昨日股票："<<last_symbols[0]<<endl;
        cout<<"平仓量:"<<need_close_num1<<endl;
        // if (last_symbols[0].substr(0,1)=="6"){
        //     start_sell("SH", last_symbols[0], 0, need_close_num1, "market");
        // }
        // else{
        //     start_sell("SZ", last_symbols[0], 0, need_close_num1, "market");
        // }

        // start_trading( last_long_contracts[0], need_close_long_num1, min_changes[last_long_symbols[0]], "close_long", exchange_dict[last_long_symbols[0]]);
        need_close  = true;
        sleep(1);
    }
    if(need_close_num2>0){
        cout<<"******************"<<endl;
        cout<<"卖出昨日股票："<<last_symbols[1]<<endl;
        cout<<"平仓量:"<<need_close_num2<<endl;
        need_close_num2 = 1000000;
        // if (last_symbols[1].substr(0,1)=="6"){
        //     start_sell("SH", last_symbols[1], 0, need_close_num2, "market");
        // }
        // else{
        //     start_sell("SZ", last_symbols[1], 0, need_close_num2, "market");
        // }


        // start_trading(last_long_contracts[1], need_close_long_num2, min_changes[last_long_symbols[1]], "close_long", exchange_dict[last_long_symbols[1]]);
        need_close  = true;
        sleep(1);
    }

    if(need_close==false) {
        cout<<"今日无需平仓"<<endl;
    }
    cout<<endl;

    int today_hold_num1 = get_contract_num(my_redis, today_symbols[0]);
    int today_hold_num2 = get_contract_num(my_redis, today_symbols[1]);

    int need_open_num1 = max(need_hold_num1 - today_hold_num1, 0);
    int need_open_num2 = max(need_hold_num2 - today_hold_num2, 0);

    cout<<"----------------------------------------------------------------------------------------------"<<endl;
    cout<<"今日信号如下："<<endl;
    cout<<"******************"<<endl;
    cout<<"待买股票1："<<today_symbols[0]<<endl;
    cout<<"当前持有量:"<<today_hold_num1<<endl;
    cout<<"需要持有量:"<<need_hold_num1<<endl;
    cout<<"需要新开仓量:"<<need_open_num1<<endl;  
    cout<<"******************"<<endl;
    cout<<"待买股票2："<<today_symbols[1]<<endl;
    cout<<"当前持有量:"<<today_hold_num2<<endl;
    cout<<"需要持有量:"<<need_hold_num2<<endl;
    cout<<"需要新开仓量:"<<need_open_num2<<endl;  


    cout<<"total_place_money="<<need_hold_money_total<<endl;
  
    if(need_open_num1>0)
    {
        if (today_symbols[0].substr(0,1)=="6"){
            start_buy("SH", today_symbols[0], 0, need_open_num1, "market");
        }
        else{
            start_buy("SZ", today_symbols[0], 0, need_open_num1, "market");
        }
        // start_trading(td_tick, long_contracts[0], need_open_long_hands1, min_changes[long_symbols[0]], "open_long", exchange_dict[long_symbols[0]]);
        sleep(1);
    }
    if(need_open_num2>0)
    {
        if (today_symbols[1].substr(0,1)=="6"){
            start_buy("SH", today_symbols[1], 0, need_open_num2, "market");
        }
        else{
            start_buy("SZ", today_symbols[1], 0, need_open_num2, "market");
        }

        //start_trading(td_tick, long_contracts[1], need_open_long_hands2, min_changes[long_symbols[1]], "open_long", exchange_dict[long_symbols[1]]);
        sleep(1);
    }

}


void Signal::make_startegy(char* *allInstruments_sh, char* *allInstruments_sz, int sh_num, int sz_num)
{
  
    string trade_date = "0";

    //load_last_data("/root/Doubleesemble/name.txt");
    load_last_data("/root/other/xtp_trade/name.txt");

    load_signal_data("/root/other/xtp_trade/name_today.txt");

    struct timeval tv;
    struct timeval tvBegin;
    struct timeval curTime;
    int* results = get_local_time();
	int minute_local = results[1];
	int sec_local = results[2];
	int microsec_local = results[3];
    struct timeval tv_ori;
    tv_ori.tv_sec = 59-sec_local;
    tv_ori.tv_usec = 1000 * (1000 - microsec_local)+100000;
    int ret = select(0, NULL, NULL, NULL, &tv_ori);
    results = get_local_time();
	minute_local = results[1];
	sec_local = results[2];
	microsec_local = results[3];
	while (1)
	{
		results = get_local_time();
		minute_local = results[1];
		sec_local = results[2];
		microsec_local = results[3];
        int  start_period =1;
	    // cout<<"minute_local="<<minute_local<<", period="<<period<<", sec_local=="<<sec_local<<", microsec_local="<<microsec_local<<endl;
		if (minute_local % start_period== 0 && sec_local == 18)
		{
			break;
		}
	}

    bool start_over = false;
    while (1)
    {
        gettimeofday(&tvBegin, NULL);
        int start_mill = tvBegin.tv_usec;
        struct timeval curTime;
        char buffer[80] = { 0 };
        char tick_min[40] = { 0 };
        struct tm nowTime;
        localtime_r(&tvBegin.tv_sec, &nowTime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &nowTime);
        strftime(tick_min, sizeof(tick_min), "%H%M", &nowTime);
        int tick_min_l = atoi(tick_min);
       
        if(tick_min_l>=1459 && tick_min_l<1501)
        {
            cout<<"start to check new singals !"<<endl;
            load_signal_data("/root/Doubleesemble/name.txt");
            break;
        }
        else{
            // check_hold_position();
            // load_signal_data("/root/Doubleesemble/name.txt", min_changes, trade_hands, exchange_dict);
            // break;
        }
        if((tick_min_l>=900 && tick_min_l<1600 ) || (tick_min_l>=2100 && tick_min_l<2400) || (tick_min_l>=0 && tick_min_l<230))
        {
            cout<<"now time="<<buffer<<", tick_min_l="<<tick_min_l<<", start_over="<<start_over<<endl;
        }
        cout<<endl;
        sleep(30);
        // check_hold_position();
        sleep(30);
    }
    exit(0);
}

