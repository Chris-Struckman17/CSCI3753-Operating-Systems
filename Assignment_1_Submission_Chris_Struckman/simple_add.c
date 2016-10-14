#include <linux/kernel.h>
#include <linux/linkage.h>
asmlinkage long sys_simple_add(int number1, int number2, int* result){
printk("Your first number is: %d\n", number1);
printk("Your second number is: %d\n", number2);
*result = number1 + number2;
printk("Your result is: %d\n", *result);

}
