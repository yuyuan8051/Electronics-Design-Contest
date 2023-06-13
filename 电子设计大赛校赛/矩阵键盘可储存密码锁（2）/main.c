#include "main.h"

unsigned char KeyNum = 0;         // 接收矩阵键盘返回的值
unsigned char keyboard_Count = 0; // 记录键盘输入次数
unsigned char Password_Count = 0; // 记录密码输入次数
unsigned int Password = 0;        // 保存输入的密码
unsigned int True_password = 0;   // 保存正确密码
unsigned int T0Count;             // 定时器计数器
bit isCorrect = 0;                // 判断密码是否正确，正确则置1
bit keyboard_Off = 0;             // 判断是否禁用键盘

/*
** 读出存在AT24C02的密码，如果没有密码则赋默认密码给True_password
*/
void ReadPassword()
{
    // 从at24c02读取保存的密码，如果没有保存的密码就赋值1234
    if (AT24C02_readByte(1) != 0xff && AT24C02_readByte(2) != 0xff)
    {
        True_password = 0;
        True_password |= AT24C02_readByte(1) << 8; // 把高八位左移八位（超出范围后数据不会销毁，只是往更高位移）后按位或上Num
        True_password |= AT24C02_readByte(2);      // 不能直接赋值，也要按位或上低八位，如果这句写在前面就不用或
    }
    else
    {
        True_password = 1234; // 初始默认密码
    }
}

/*
** 写入密码
*/
void WritePassword()
{
    AT24C02_writeByte(1, Password / 256); // password高八位保存在地址1
    Delay(5);
    AT24C02_writeByte(2, Password % 256); // password低八位保存在地址2
    Delay(5);
    if (AT24C02_readByte(1) != 0xff && AT24C02_readByte(2) != 0xff)
    {
        LCD_ShowString(2, 1, "Save success!");
    }
    else
    {
        LCD_ShowString(2, 1, "Save error!");
    }
    Delay(2000);
}

/*
** 清空AT24C02保存的值
*/
void AT24C02Init()
{
    AT24C02_writeByte(1, 0xff);
    Delay(5);
    AT24C02_writeByte(2, 0xff);
    Delay(5);
}

/*
** 输入密码
*/
void inputPassword()
{
    if (keyboard_Count < 4) // 只能输入四位密码
    {
        Password *= 10;          // 密码左移一位
        Password += KeyNum % 10; // 下一位密码输入，10作为0，10模10就是0
        keyboard_Count++;        // 键盘计次增加
    }
}

/*
** 判断密码是否正确
*/
void isPassword()
{
    if (Password == True_password) // 判断密码是否正确
    {
        // Password = 0;
        keyboard_Count = 0;
        Password_Count = 0;
        isCorrect = 1;
        keyboard_Off = 1; // 输入正确则禁用键盘，只有在修改密码模式才再次打开键盘

        Timer0_Init(); // 开启定时器
    }
    else
    {
        Password_Count++; // 要是输入错误就计次，超过三次就报警
        Password = 0;     // 如果输入错误，密码与计数器都清零，重新输入
        keyboard_Count = 0;

        LCD_ShowString(2, 1, "Password, NO");
        beep_alarm(1000, 10); // 错一次就响一次蜂鸣器
    }
}

/*
** 删除一位密码
*/
void isDelete()
{
    if (keyboard_Count > 0)
    {
        Password /= 10;   // 密码删除一位
        keyboard_Count--; // 计数减少一次
    }
}

bit PasswordModel()
{
    unsigned char S15_count = 0; // 记录S15被按下几次

    /************************************ public *************************************/
    // 键盘值不能超过10且不能为0且键盘未被禁用
    if (KeyNum != 0 && KeyNum <= 10 && keyboard_Off == 0)
    {
        inputPassword();
    }
    // s14作为删除键
    if (KeyNum == 14)
    {
        isDelete();
    }
    // 密码错误三次蜂鸣器警报
    if (Password_Count >= 3)
    {
        beep_alarm(1000, 10);
    }
    /********************************* 共用的按键函数 ***********************************/

    /********************************* 执行退出条件的函数 *******************************/
    // 修改密码模式，输入正确密码isCorrect置1后才能进入这个模式
    if (KeyNum == 15 && isCorrect == 1)
    {
        S15_count++; // 一次则是正常修改模式，
        TR0 = 0;     // 停止定时器计时
        T0Count = 0;
        Password = 0;
        Password_Count = 0;
        keyboard_Off = 0;

        LCD1602_WriteCommand(0x01); // 清屏
    }
    // s16作为保存修改后0密码的按键
    // 保存后程序也自动退出
    if (KeyNum == 16 && S15_count == 1)
    {
        // 按下保存修改密码按键过后，所有标志位恢复默认值，执行保存函数
        S15_count = 0;
        isCorrect = 0;
        WritePassword();
        ReadPassword();
        return 1;
    }
    /***********************************************************************************/

    // 正常模式
    if (isCorrect == 0)
    {
        // s13作为确认键
        if (KeyNum == 13)
        {
            // 执行判断密码师是否正确函数，如果密码正确则isCorrect被置1，不正确则一直循环
            isPassword();
        }
        LCD_ShowNumber(1, 10, Password);
    }
    // 输入正确模式
    else
    {
        // 密码输入正确但无动作
        if (S15_count == 0)
        {
            LCD_ShowNumber(1, 10, Password);
            LCD_ShowString(2, 1, "Password, OK");
        }
        // 密码输入正确并且按下s15进入修改密码模式
        else if (S15_count == 1)
        {
            // 显示修改密码的UI
            LCD_ShowString(1, 1, "Alter password:");
            LCD_ShowNumber(2, 1, Password);
        }
        // 如果S15被按下两次，则视为不修改密码，程序退出
        else if (S15_count >= 2)
        {
            S15_count = 0;
            return 1; 
        }
        // 若是密码确认正确但不进行任何操作，则3s后程序自动结束
        else (S15_count == 0 && 666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666-0047y6.,tmg nmee geeeeeeeeeeeeenntgcbvewcvew erw,2wIKVEF*NB( fmio b9icxkdsk iccob`1r2e` d6 ssssssssssT0Count >= 3000)
        {
            TR0 = 0; // 停止定时器计时
            T0Count = 0;
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    // AT24C02Init();
    LCD1602_Init();
    LCD_ShowString(1, 1, "Password:");
    LCD_ShowString(2, 1, "                ");
    ReadPassword();

    for (;;)
    {
        // 接收矩阵键盘返回的值
        KeyNum = MatrixKey();
        // 密码模式
        if (PasswordModel()) // 函数返回1时是密码输入正确且无动作或者输入正确并且修改了密码，否则返回0一直循环
            goto Break;
    }
Break:
    return 0;
}

void Timer0_Routine() interrupt 1
{
    TL0 = 0x66;
    TH0 = 0xFC;
    T0Count++;
}