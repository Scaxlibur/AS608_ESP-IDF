/*******************************************************************************
****文件路径         : .\main\fingerID.cpp
****作者名称         : Scaxlibur
****文件版本         : V1.0.0
****创建日期         : 2024-10-21 20:25:26
****简要说明         : 指纹识别模块
********************************************************************************/

#include "fingerID.hpp"

IDENTIFIER::IDENTIFIER(void){
    init_uart2id();
    printf("指纹识别器对象已创建\n");
    vTaskDelay(200 / portTICK_PERIOD_MS);
    AS608_Check();
}

void IDENTIFIER::init_uart2id(void){
    if(uart_is_driver_installed(UART_NUM_ID) == false)//注册驱动前检查，避免重复注册报错
    {
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_ID, &uart2id_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_ID, UART_NUM_ID_TX, UART_NUM_ID_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_ID, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    };
}

void IDENTIFIER::IDUARTwrite_Bytes(uint8_t data)
{
    uart_write_bytes(UART_NUM_ID, &data, 1);
}


void IDENTIFIER::IDUARTwrite_Bytes(uint16_t data)
{
    uint8_t data1 = data >> 8;
    uart_write_bytes(UART_NUM_ID, &data1, 1);
    uart_write_bytes(UART_NUM_ID, &data, 1);
}

void IDENTIFIER::IDUARTwrite_Bytes(uint32_t data)
{
    uint8_t data1 = data >> 24;
    uint8_t data2 = data >> 16;
    uint8_t data3 = data >> 8;
    uart_write_bytes(UART_NUM_ID, &data1, 1);
    uart_write_bytes(UART_NUM_ID, &data2, 1);
    uart_write_bytes(UART_NUM_ID, &data3, 1);
    uart_write_bytes(UART_NUM_ID, &data, 1);
}

void IDENTIFIER::SendHead(void)
{
    ESP_ERROR_CHECK(uart_flush(UART_NUM_ID));
    IDUARTwrite_Bytes(PACKHEAD);
}

void IDENTIFIER::SendAddr(void)
{

    IDUARTwrite_Bytes(IDaddr);
}

void IDENTIFIER::SendFlag(uint8_t flag)
{
    IDUARTwrite_Bytes(flag);
}

void IDENTIFIER::SendLength(uint16_t length)
{
  IDUARTwrite_Bytes(length);
}

void IDENTIFIER::Sendcmd(uint8_t cmd)
{
  IDUARTwrite_Bytes(cmd);
}

void IDENTIFIER::SendCheck(uint16_t check)
{
  IDUARTwrite_Bytes(check);
}

/*****************************************
函数名：bool AS608_Check(void)
参数：无
功能描述：模块是否连接检测 
返回值：模块连接了返回true 否则返回false
*****************************************/
bool IDENTIFIER::AS608_Check(void)
{
    uart_flush(UART_NUM_ID);
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);
    SendLength((uint16_t)0x07);
    Sendcmd((uint8_t)0x13);
    IDUARTwrite_Bytes(IDpwd);
    uint16_t sum = 0x07 + 0x13 + IDpwd;
    SendCheck(sum); 
    
    vTaskDelay(200 / portTICK_PERIOD_MS);//等待200ms
    //PS_GetRandomCode();
    size_t bufferLenth = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_ID, &bufferLenth));
    if(bufferLenth == 0){
        #ifdef TEST
        printf("AS608连接失败\n");
        #endif  
        return false;
    }
    #ifdef TEST
        printf("AS608连接成功\n");
    #endif    
    return true;
}

uint8_t* IDENTIFIER::JudgeStr()
{
        #ifdef TEST
        printf("JudgeStr()调用\n");
        #endif
    uint8_t receive[16];
    uint8_t str[8];
    str[0] = 0xEF;
    str[1] = 0x01;
    str[2] = IDaddr >> 24;
    str[3] = IDaddr >> 16;
    str[4] = IDaddr >> 8;
    str[5] = IDaddr;
    str[6] = 0x07;
    str[7] = '\0';
    size_t uartSize;
    vTaskDelay(500/portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_ID, &uartSize));
        #ifdef TEST
        printf("接收缓冲区%x字节\n", uartSize);
        #endif
    uart_read_bytes(UART_NUM_ID, receive, uartSize, 500/portTICK_PERIOD_MS);
    return (uint8_t *)strstr((const char*)receive, (const char*)str);//错误在这一行
}

//录入图像 PS_GetImage
//功能:探测手指，探测到后录入指纹图像存于ImageBuffer。
//模块返回确认字
uint8_t IDENTIFIER::PS_GetImage(void)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);
    SendLength(0x03);
    Sendcmd(0x01);
    sum =  0x01 + 0x03 + 0x01;
    SendCheck(sum);
    data = JudgeStr();
    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//生成特征 PS_GenChar
//功能:将ImageBuffer中的原始图像生成指纹特征文件存于CharBuffer1或CharBuffer2
//参数:BufferID --> charBuffer1:0x01	charBuffer1:0x02
//模块返回确认字
uint8_t IDENTIFIER::PS_GenChar(uint8_t BufferID)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);
    SendLength(0x04);
    Sendcmd(0x02);
    IDUARTwrite_Bytes(BufferID);
    sum = 0x01 + 0x04 + 0x02 + BufferID;
    SendCheck(sum);
    data = JudgeStr();
    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//精确比对两枚指纹特征 PS_Match
//功能:精确比对CharBuffer1 与CharBuffer2 中的特征文件
//模块返回确认字
uint8_t IDENTIFIER::PS_Match(void)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);
    SendLength(0x03);
    Sendcmd(0x03);
    sum = 0x01 + 0x03 + 0x03;
    SendCheck(sum);
    data = JudgeStr();
    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//搜索指纹 PS_Search
//功能:以CharBuffer1或CharBuffer2中的特征文件搜索整个或部分指纹库.若搜索到，则返回页码。
//参数:  BufferID @ref CharBuffer1	CharBuffer2
//说明:  模块返回确认字，页码（相配指纹模板）
uint8_t IDENTIFIER::PS_Search(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);//命令包标识
    SendLength(0x08);
    Sendcmd(0x04);
    IDUARTwrite_Bytes(BufferID);
    IDUARTwrite_Bytes(StartPage);
    IDUARTwrite_Bytes(PageNum);
    sum = 0x01 + 0x08 + 0x04 + BufferID
            + (StartPage >> 8) + (uint8_t)StartPage
            + (PageNum >> 8) + (uint8_t)PageNum;
    SendCheck(sum);
    data = JudgeStr();    
    if(data)
    {
        ensure = data[9];
        p->pageID   = (data[10] << 8) + data[11];
        p->mathscore = (data[12] << 8) + data[13];
    }
    else
        ensure = 0xff;
    return ensure;
}

//合并特征（生成模板）PS_RegModel
//功能:将CharBuffer1与CharBuffer2中的特征文件合并生成 模板,结果存于CharBuffer1与CharBuffer2
//说明:  模块返回确认字
uint8_t IDENTIFIER::PS_RegModel(void)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);//命令包标识
    SendLength(0x03);
    Sendcmd(0x05);
    sum = 0x01 + 0x03 + 0x05;
    SendCheck(sum);
    data = JudgeStr();    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//储存模板 PS_StoreChar
//功能:将 CharBuffer1 或 CharBuffer2 中的模板文件存到 PageID 号flash数据库位置。
//参数:  BufferID @ref charBuffer1:0x01	charBuffer1:0x02
//       PageID（指纹库位置号）
//说明:  模块返回确认字
uint8_t IDENTIFIER::PS_StoreChar(uint8_t BufferID, uint16_t PageID)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x06);
    Sendcmd(0x06);
    IDUARTwrite_Bytes(BufferID);
    IDUARTwrite_Bytes(PageID);
    sum = 0x01 + 0x06 + 0x06 + BufferID
            + (PageID >> 8) + (uint8_t)PageID;
    SendCheck(sum);
    data = JudgeStr();    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//删除模板 PS_DeletChar
//功能:  删除flash数据库中指定ID号开始的N个指纹模板
//参数:  PageID(指纹库模板号)，N删除的模板个数。
//说明:  模块返回确认字
uint8_t IDENTIFIER::PS_DeletChar(uint16_t PageID, uint16_t N)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x07);
    Sendcmd(0x0C);
    IDUARTwrite_Bytes(PageID);
    IDUARTwrite_Bytes(N);
    sum = 0x01 + 0x07 + 0x0C
            + (PageID >> 8) + (uint8_t)PageID
            + (N >> 8) + (uint8_t)N;
    SendCheck(sum);
    data = JudgeStr();    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//清空指纹库 PS_Empty
//功能:  删除flash数据库中所有指纹模板
//参数:  无
//说明:  模块返回确认字
uint8_t IDENTIFIER::PS_Empty(void)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x03);
    Sendcmd(0x0D);
    sum = 0x01 + 0x03 + 0x0D;
    SendCheck(sum);
    data = JudgeStr();    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//写系统寄存器 PS_WriteReg
//功能:  写模块寄存器
//参数:  寄存器序号RegNum:4\5\6
//说明:  模块返回确认字
uint8_t IDENTIFIER::PS_WriteReg(uint8_t RegNum, uint8_t DATA)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x05);
    Sendcmd(0x0E);
    IDUARTwrite_Bytes(RegNum);
    IDUARTwrite_Bytes(DATA);
    sum = RegNum + DATA + 0x01 + 0x05 + 0x0E;
    SendCheck(sum);
    data = JudgeStr();    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    /*
    if(ensure == 0)
        //printf("\r\n设置参数成功！");
    else
        //printf("\r\n%s", EnsureMessage(ensure));
    */
    return ensure;
}

//读系统基本参数 PS_ReadSysPara
//功能:  读取模块的基本参数（波特率，包大小等)
//参数:  无
//说明:  模块返回确认字 + 基本参数（16bytes）
uint8_t IDENTIFIER::PS_ReadSysPara(SysPara *p)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x03);
    Sendcmd(0x0F);
    sum = 0x01 + 0x03 + 0x0F;
    SendCheck(sum);
    data = JudgeStr();    if(data)
    {
        ensure = data[9];
        p->PS_max = (data[14] << 8) + data[15];
        p->PS_level = data[17];
        p->PS_addr = (data[18] << 24) + (data[19] << 16) + (data[20] << 8) + data[21];
        p->PS_size = data[23];
        p->PS_N = data[25];
    }
    else
        ensure = 0xff;
    if(ensure == 0x00)
    {
        printf("\r\n模块最大指纹容量=%d", p->PS_max);
        printf("\r\n对比等级=%d", p->PS_level);
        //printf("\r\n地址=%x", p->PS_addr);
        printf("\r\n波特率=%d", p->PS_N * 9600);
    }
    else
        printf("\r\n%s", EnsureMessage(ensure));
    return ensure;
}

//设置模块地址 PS_SetAddr
//功能:  设置模块地址
//参数:  PS_addr
//说明:  模块返回确认字
uint8_t IDENTIFIER::PS_SetAddr(uint32_t PS_addr)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);
    SendLength((uint16_t)0x07);
    Sendcmd((uint8_t)0x15);
    IDUARTwrite_Bytes(PS_addr >> 24);
    IDUARTwrite_Bytes(PS_addr >> 16);
    IDUARTwrite_Bytes(PS_addr >> 8);
    IDUARTwrite_Bytes(PS_addr);
    sum = 0x01 + 0x07 + 0x15
            + (uint8_t)(PS_addr >> 24) + (uint8_t)(PS_addr >> 16)
            + (uint8_t)(PS_addr >> 8) + (uint8_t)PS_addr;
    SendCheck(sum);
    IDaddr = PS_addr; //发送完指令，更换地址
    data = JudgeStr();    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    IDaddr = PS_addr;
    if(ensure == 0x00)
        printf("\r\n设置地址成功！");
    else
        printf("\r\n%s", EnsureMessage(ensure));
    return ensure;
}

//功能： 模块内部为用户开辟了256bytes的FLASH空间用于存用户记事本,
//	该记事本逻辑上被分成 16 个页。
//参数:  NotePageNum(0~15),Byte32(要写入内容，32个字节)
//说明:  模块返回确认字
uint8_t IDENTIFIER::PS_WriteNotepad(uint8_t NotePageNum, uint8_t *Byte32)
{
    uint16_t sum = 0;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(36);
    Sendcmd(0x18);
    IDUARTwrite_Bytes(NotePageNum);
    for(uint8_t i = 0; i < 32; i++)
    {
        IDUARTwrite_Bytes(Byte32[i]);
        sum = sum + Byte32[i];
    }
    sum = 0x01 + 36 + 0x18 + NotePageNum + sum;
    SendCheck(sum);
    data = JudgeStr();    if(data)
        ensure = data[9];
    else
        ensure = 0xff;
    return ensure;
}

//读记事PS_ReadNotepad
//功能：  读取FLASH用户区的128bytes数据
//参数:  NotePageNum(0~15)
//说明:  模块返回确认字+用户信息
uint8_t IDENTIFIER::PS_ReadNotepad(uint8_t NotePageNum, uint8_t *Byte32)
{
    uint16_t sum;
    uint8_t  ensure, i;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x04);
    Sendcmd(0x19);
    IDUARTwrite_Bytes(NotePageNum);
    sum = 0x01 + 0x04 + 0x19 + NotePageNum;
    SendCheck(sum);
    data = JudgeStr();    if(data)
    {
        ensure = data[9];
        for(i = 0; i < 32; i++)
        {
        Byte32[i] = data[10 + i];
        }
    }
    else
        ensure = 0xff;
    return ensure;
}

//高速搜索PS_HighSpeedSearch
//功能：以 CharBuffer1或CharBuffer2中的特征文件高速搜索整个或部分指纹库。
//		  若搜索到，则返回页码,该指令对于的确存在于指纹库中 ，且登录时质量
//		  很好的指纹，会很快给出搜索结果。
//参数:  BufferID， StartPage(起始页)，PageNum（页数）
//说明:  模块返回确认字+页码（相配指纹模板）
uint8_t IDENTIFIER::PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x08);
    Sendcmd(0x1b);
    IDUARTwrite_Bytes(BufferID);
    IDUARTwrite_Bytes(StartPage);
    IDUARTwrite_Bytes(PageNum);
    sum = 0x01 + 0x08 + 0x1b + BufferID
            + (StartPage >> 8) + (uint8_t)StartPage
            + (PageNum >> 8) + (uint8_t)PageNum;
    SendCheck(sum);
    data = JudgeStr();    if(data)
    {
        ensure = data[9];
        p->pageID 	= (data[10] << 8) + data[11];
        p->mathscore = (data[12] << 8) + data[13];
    }
    else
        ensure = 0xff;
    return ensure;
}

//读有效模板个数 PS_ValidsumleteNum
//功能：读有效模板个数
//参数: 无
//说明: 模块返回确认字+有效模板个数ValidN
uint8_t IDENTIFIER::PS_ValidTempleteNum(uint16_t *ValidN)
{
    uint16_t sum;
    uint8_t  ensure;
    uint8_t  *data = NULL;
    SendHead();
    SendAddr();
    SendFlag(0x01);//命令包标识
    SendLength(0x03);
    Sendcmd(0x1d);
    sum = 0x01 + 0x03 + 0x1d;
    SendCheck(sum);
    data = JudgeStr();    if(data)
    {
        ensure = data[9];
        *ValidN = (data[10] << 8) + data[11];
    }
    else
        ensure = 0xff;
    
    if(ensure == 0x00)
    {
        printf("\r\n有效指纹个数=%d", (data[10] << 8) + data[11]);
    }
    else
        printf("\r\n%s", EnsureMessage(ensure));
    return ensure;
}

//与AS608握手 PS_HandShake
//参数: PS_Addr地址指针
//说明: 初始化必做
uint8_t IDENTIFIER::PS_HandShake(uint32_t *PS_Addr)
{
    SendHead();
    SendAddr();
    IDUARTwrite_Bytes(COMMANDSIGN);
    IDUARTwrite_Bytes((uint8_t)0X00);
    IDUARTwrite_Bytes((uint8_t)0X00);
    //vTaskDelay(200 / portTICK_PERIOD_MS);
    size_t uartSize;
    uint8_t data[16];
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_ID, &uartSize));
    uart_read_bytes(UART_NUM_ID, &data, uartSize, 200/portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(uart_flush(UART_NUM_ID));
    if(uartSize != 0) //接收到数据
    {
        if(
        data[0] == 0XEF
        && data[1] == 0X01
        && data[6] == 0X07
        )
        {
        *PS_Addr = (data[2] << 24) + (data[3] << 16)
                    + (data[4] << 8) + (data[5]);
        return 0;
        }
    }
    return 1;
}

//模块应答包确认码信息解析
//功能：解析确认码错误信息返回信息
//参数: ensure
const char* IDENTIFIER::EnsureMessage(uint8_t ensure)
{
    const char *p;
    switch(ensure)
    {
    case  0x00:
        p = "       OK       ";
        break;
    case  0x01:
        p = " 数据包接收错误 ";
        break;
    case  0x02:
        p = "传感器上没有手指";
        break;
    case  0x03:
        p = "录入指纹图像失败";
        break;
    case  0x04:
        p = " 指纹太干或太淡 ";
        break;
    case  0x05:
        p = " 指纹太湿或太糊 ";
        break;
    case  0x06:
        p = "  指纹图像太乱  ";
        break;
    case  0x07:
        p = " 指纹特征点太少 ";
        break;
    case  0x08:
        p = "  指纹不匹配    ";
        break;
    case  0x09:
        p = " 没有搜索到指纹 ";
        break;
    case  0x0a:
        p = "   特征合并失败 ";
        break;
    case  0x0b:
        p = "地址序号超出范围";
    case  0x10:
        p = "  删除模板失败  ";
        break;
    case  0x11:
        p = " 清空指纹库失败 ";
        break;
    case  0x15:
        p = "缓冲区内无有效图";
        break;
    case  0x18:
        p = " 读写FLASH出错  ";
        break;
    case  0x19:
        p = "   未定义错误   ";
        break;
    case  0x1a:
        p = "  无效寄存器号  ";
        break;
    case  0x1b:
        p = " 寄存器内容错误 ";
        break;
    case  0x1c:
        p = " 记事本页码错误 ";
        break;
    case  0x1f:
        p = "    指纹库满    ";
        break;
    case  0x20:
        p = "    地址错误    ";
        break;
    default :
        p = " 返回确认码有误 ";
        break;
    }
    return p;
}

//显示确认码错误信息
void IDENTIFIER::ShowErrMessage(uint8_t ensure)
{
    //OLED_ShowCH(5,0,(uint8_t*)EnsureMessage(ensure));
	printf("%s\r\n",EnsureMessage(ensure));
}
 
 
//录指纹
void IDENTIFIER::Add_FR(void)
{
    uint8_t i = 0;
    uint8_t ensure, processnum = 0;
    uint8_t ID_NUM = 0;
    while(1)
    {
        switch (processnum)
        {
        case 0:
        i++;
                printf("请按手指\r\n");
        ensure = PS_GetImage();
        if(ensure == 0x00)
        {
            ensure = PS_GenChar(CharBuffer1); //生成特征
            if(ensure == 0x00)
            {
                        printf("指纹正常\r\n");
            i = 0;
            processnum = 1; //跳到第二步
            }
            else ShowErrMessage(ensure);
        }
        else ShowErrMessage(ensure);
        break;
    
        case 1:
        i++;
            printf("请再按一次\r\n");
        ensure = PS_GetImage();
        if(ensure == 0x00)
        {
            ensure = PS_GenChar(CharBuffer2); //生成特征
            if(ensure == 0x00)
            {
                        printf("指纹正常\r\n");
            i = 0;
            processnum = 2; //跳到第三步
            }
            else ShowErrMessage(ensure);
        }
        else ShowErrMessage(ensure);
        break;
    
        case 2:
            printf("对比两次指纹\r\n");
        ensure = PS_Match();
        if(ensure == 0x00)
        {
                    printf("对比成功\r\n");
            processnum = 3; //跳到第四步
        }
        else
        {
                    printf("对比失败\r\n");
            ShowErrMessage(ensure);
            i = 0;
            processnum = 0; //跳回第一步
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        break;
    
        case 3:
            printf("生成指纹模板\r\n");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        ensure = PS_RegModel();
        if(ensure == 0x00)
        {
                    printf("生成指纹模板成功\r\n");
            processnum = 4; //跳到第五步
        }
        else
        {
            processnum = 0;
            ShowErrMessage(ensure);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        break;
    
        case 4:
                printf("默认选择ID为1 \r\n");
            ID_NUM = 1;
            #if 0
        while(key_num != 3)
        {
            key_num = KEY_Scan(0);
            if(key_num == 2)
            {
            key_num = 0;
            if(ID_NUM > 0)
                ID_NUM--;
            }
            if(key_num == 4)
            {
            key_num = 0;
            if(ID_NUM < 99)
                ID_NUM++;
            }
            OLED_ShowCH(40, 6, "ID=");
            OLED_ShowNum(65, 6, ID_NUM, 2, 1);
        }
            
        key_num = 0;
                #endif
        ensure = PS_StoreChar(CharBuffer2, ID_NUM); //储存模板
        if(ensure == 0x00)
        {
                    printf("录入指纹成功\r\n");
            vTaskDelay(1500 / portTICK_PERIOD_MS);
            return ;
        }
        else
        {
            processnum = 0;
            ShowErrMessage(ensure);
        }
        break;
        }
        vTaskDelay(400 / portTICK_PERIOD_MS);
        if(i == 10) //超过5次没有按手指则退出
        {
        break;
        }
    }
}
 
//SysPara AS608Para;//指纹模块AS608参数

//刷指纹
void IDENTIFIER::press_FR(void)
{
    SearchResult seach;
    uint8_t ensure;
    char str[20];
    while(1)
    {
        ensure = PS_GetImage();
        if(ensure == 0x00) //获取图像成功
        {
        ensure = PS_GenChar(CharBuffer1);
        if(ensure == 0x00) //生成特征成功
        {
            ensure = PS_HighSpeedSearch(CharBuffer1, 0, 99, &seach);
            if(ensure == 0x00) //搜索成功
            {
                        printf("指纹验证成功");
            //sprintf(str, " ID:%d 得分:%l ", seach.pageID, seach.mathscore);
                        printf("%s\r\n",str);
            vTaskDelay(1500 / portTICK_PERIOD_MS);
            }
            else
            {
                printf("验证失败\r\n");
            vTaskDelay(1500 / portTICK_PERIOD_MS);
            }
        }
        else
                {};
                printf("请按手指\r\n");
        }
    }
}
 
//删除单个指纹
void IDENTIFIER::Del_FR(void)
{
    uint8_t  ensure;
    uint16_t ID_NUM = 0;
        printf("单个删除指纹开始，默认删除ID为1");
        ID_NUM = 1;
    ensure = PS_DeletChar(ID_NUM, 1); //删除单个指纹
    if(ensure == 0)
    {
        printf("删除指纹成功 \r\n");
    }
    else
        ShowErrMessage(ensure);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    
    }
    /*清空指纹库*/
    void IDENTIFIER::Del_FR_Lib(void)
    {
        uint8_t  ensure;
        printf("删除指纹库开始\r\n");
    ensure = PS_Empty(); //清空指纹库
    if(ensure == 0)
    {
            printf("清空指纹库成功\r\n");
    }
    else
        ShowErrMessage(ensure);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
}

uint32_t IDENTIFIER::PS_GetRandomCode()
{
    uint8_t  ensure;
    uint8_t *data = NULL;
    uint32_t randomCode = 0;
    SendHead();
    SendAddr();
    SendFlag(COMMANDSIGN);
    SendLength(0x03);
    Sendcmd(0x14);
    SendCheck(0x18);
    data = JudgeStr();    
    if(data)
    {
        ensure = data[9];
        randomCode = (data[10] << 24)+
                     (data[11] << 16)+
                     (data[12] << 8)+
                      data[13];
    }
    else
        ensure = 0x01;
    
    if(ensure == 0x00)
    {
        #ifdef TEST
        printf("\r随机数%lx\n", randomCode);
        #endif
    }
    else
        #ifdef TEST
        printf("\r\n%s", EnsureMessage(ensure));
        #endif
    return randomCode;
}