#include <linux/init.h>
#include <linux/module.h>
#include <linux/irq.h>  //中断相关
#include <linux/interrupt.h> //中断相关
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/input.h> //定义了按键的标准键值

//声明描述按键硬件信息的数据结构
struct btn_resource {
    char *name; //按键名称
    int irq; //中断号
    int gpio; //GPIO编号(管脚为复用IO)
    int code; //按键值
};

//定义初始化两个按键信息对象
static struct btn_resource btn_info[] = {
    {
        .name = "KEY_UP",
        .irq = IRQ_EINT(0),
        .gpio = S5PV210_GPH0(0),
        .code = KEY_UP
    },
    {
        .name = "KEY_DOWN",
        .irq = IRQ_EINT(1),
        .gpio = S5PV210_GPH0(1),
        .code = KEY_DOWN
    }
};

//记录按键的硬件信息
static struct btn_resource *pdata; 

//tasklet延后处理函数
//理论上做不仅仅,耗时较长的内容
static void btn_tasklet_function(unsigned long data)
{
    int state;
    
    //1.获取按键的操作状态
    state = gpio_get_value(pdata->gpio);
   
    //2.打印按键信息
    printk("%s: g_data = %#x,按键%s的状态为%s,键值为%d\n",
            __func__, *(int *)data, pdata->name, 
            state ?"松开":"按下",
            pdata->code);
}

static int g_data = 0x5555;

//定义初始化tasklet对象
static DECLARE_TASKLET(btn_tsk, //对象名 
                btn_tasklet_function,//延后处理函数
                (unsigned long)&g_data //传参
                );

//顶半部：中断处理函数
//一旦中断处理函数注册到内核,将来按键有操作给CPU产生
//中断,内核就会调用注册的中断处理函数
//切记：中断处理函数所做的内容完全依赖用户需求
static irqreturn_t button_isr(int irq, void *dev_id)
{
    //1.通过dev_id获取对应的按键硬件信息
    //KEY_UP有操作,pdata指向btn_info[0]
    //KEY_DOWN有操作,pdata指向btn_info[1]
    pdata = (struct btn_resource *)dev_id;

    //2.登记tasklet延后处理函数
    //一旦登记完毕,内核在适当的时候执行延后处理函数
    tasklet_schedule(&btn_tsk);

    printk("顶半部：%s\n", __func__);
    return IRQ_HANDLED; 
    //正常处理中断,IRQ_NONE:中断处理异常
}

static int btn_init(void)
{
    int i;

    //1.申请GPIO资源
    //2.申请中断资源和注册中断处理函数到内核
    //多个按键共享一个中断处理函数
    for (i = 0; i < ARRAY_SIZE(btn_info); i++) {
        gpio_request(btn_info[i].gpio,
                        btn_info[i].name);
        request_irq(btn_info[i].irq,
                    button_isr,
        IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                    btn_info[i].name,
                    &btn_info[i]);
    }
    return 0;
}

static void btn_exit(void)
{
    int i;
    //1.删除中断处理函数,释放中断资源和GPIO资源
    for (i = 0; i < ARRAY_SIZE(btn_info); i++) {
        free_irq(btn_info[i].irq, &btn_info[i]);
        gpio_free(btn_info[i].gpio);
    } 
}
module_init(btn_init);
module_exit(btn_exit);
MODULE_LICENSE("GPL");
