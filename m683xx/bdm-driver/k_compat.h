/*
 * k_compat.h 1.8 1995/12/03 01:26:43 (David Hinds)
 *                1998/07/12 modified by Pavel Pisa pisa@CMP.felk.cvut.cz
 */

#ifndef _LINUX_K_COMPAT_H
#define _LINUX_K_COMPAT_H

#define VERSION(v,p,s) (((v)<<16)+(p<<8)+s)

#ifndef LINUX_VERSION_CODE
  #error LINUX_VERSION_CODE not defined
#endif

#if (LINUX_VERSION_CODE < VERSION(1,3,38))

#ifdef MODULE
#include <linux/module.h>
#if !defined(CONFIG_MODVERSIONS) && !defined(__NO_VERSION__)
char kernel_version[] = UTS_RELEASE;
#endif
#else
#define MOD_DEC_USE_COUNT
#define MOD_INC_USE_COUNT
#endif

#else /* 1.3.38 */

#if (LINUX_VERSION_CODE <= VERSION(2,5,60))
#ifdef CONFIG_MODVERSIONS
#define MODVERSIONS 1
#if (LINUX_VERSION_CODE >= VERSION(1,3,40)) 
#include <linux/modversions.h>
#endif /* >=1.3.40 */
#endif /* CONFIG_MODVERSIONS */
#endif /* <=2.5.60 */

#include <linux/module.h>

#endif /* 1.3.38 */

#if (LINUX_VERSION_CODE < VERSION(2,5,50))
#define kc_MOD_DEC_USE_COUNT MOD_DEC_USE_COUNT
#define kc_MOD_INC_USE_COUNT MOD_INC_USE_COUNT
#else /* 2.5.50 */
#define kc_MOD_DEC_USE_COUNT
#define kc_MOD_INC_USE_COUNT
#endif /* 2.5.50 */

#if (LINUX_VERSION_CODE < VERSION(2,2,0)) 
  #ifndef EXPORT_NO_SYMBOLS
    #define EXPORT_NO_SYMBOLS
  #endif /*EXPORT_NO_SYMBOLS*/
#endif /* 2.2.0 */

/*** user memory access ***/

/* I do not know exactly all dates of changes */

#if (LINUX_VERSION_CODE < VERSION(2,1,5))
  #define kc_copy_from_user memcpy_fromfs
  #define kc_copy_to_user   memcpy_tofs
  #define kc_get_user_long  get_fs_long
  #define kc_get_user_word  get_fs_word
  #define kc_get_user_byte  get_fs_byte
  #define kc_put_user_long  put_fs_long
  #define kc_put_user_word  put_fs_word
  #define kc_put_user_byte  put_fs_byte
#elif (LINUX_VERSION_CODE < VERSION(2,1,100)) /* may need correction */
  #include  <asm/uaccess.h>
  #define kc_copy_from_user copy_from_user
  #define kc_copy_to_user   copy_to_user
  #define kc_get_user(ptr) \
    ({ __typeof__(*(ptr)) x; \
       __get_user_check(x,(ptr),sizeof(*(ptr))); x; })
  #define kc_put_user(x,ptr) \
    (__put_user_check((x),ptr,sizeof(*(ptr))))
#else /* >= 2.1.100 */
  #include  <asm/uaccess.h>
  #define kc_copy_from_user copy_from_user
  #define kc_copy_to_user   copy_to_user
  #define kc_get_user(ptr) \
    ({ __typeof__(*(ptr)) x; \
       get_user(x,(ptr)); x; })
  #define kc_put_user(x,ptr) \
    (put_user((x),(ptr)))
#endif /* < 2.1.100 */

#if (LINUX_VERSION_CODE >= VERSION(2,1,5))
  #define kc_get_user_long(ptr)   (kc_get_user((long*)(ptr)))
  #define kc_get_user_word(ptr)   (kc_get_user((unsigned short*)(ptr)))
  #define kc_get_user_byte(ptr)   (kc_get_user((unsigned char*)(ptr)))
  #define kc_put_user_long(x,ptr) (kc_put_user((x),(long *)(ptr)))
  #define kc_put_user_word(x,ptr) (kc_put_user((x),(unsigned short*)(ptr)))
  #define kc_put_user_byte(x,ptr) (kc_put_user((x),(unsigned char*)(ptr)))
#endif /* >= 2.1.5 */

/*** bitops changes ***/

#if (LINUX_VERSION_CODE < VERSION(2,1,36)) /* may need correction */
   #define test_and_set_bit set_bit
#endif

/*** interrupt related stuff ***/

#if (LINUX_VERSION_CODE < VERSION(2,1,36)) /* may need correction */
   #define kc_synchronize_irq(irqnum) do{cli();sti();}while(0)
#elif (LINUX_VERSION_CODE < VERSION(2,5,33)) /* may need correction */
   #define kc_synchronize_irq(irqnum) synchronize_irq()
#else /* >=2.5.33 */
   #define kc_synchronize_irq synchronize_irq
#endif

#if (LINUX_VERSION_CODE <= VERSION(2,5,67)) && !defined(IRQ_RETVAL)
   typedef void irqreturn_t;
   #define IRQ_NONE
   #define IRQ_HANDLED
   #define IRQ_RETVAL(x)
#endif /* <=2.5.67 */

/*** timming related stuff ***/

#if (LINUX_VERSION_CODE < VERSION(2,1,100)) /* needs correction */
   #define schedule_timeout(timeout_jif) ({ \
       current->timeout = jiffies + (timeout_jif); \
       schedule(); \
       current->timeout = 0; \
   })
#endif

#if (LINUX_VERSION_CODE < VERSION(2,1,36)) /* needs correction */
   #define mod_timer(timer,expires) ({\
       del_timer(timer); \
       timer->expires=expires; \
       add_timer(timer); \
   })
#endif

/*** file_operations changes ***/

#if (LINUX_VERSION_CODE < VERSION(2,1,5))
   #define CLOSERET   void
   #define RWRET      int
   #define RWCOUNT_T  int
   #define RWINODE_P  struct inode *inode,
   #define RWPPOS_P
   #define RWINODE    inode
   #define KC_FOPS_FLUSH(ptr)
   #define kc_dev2minor MINOR
#elif (LINUX_VERSION_CODE < VERSION(2,1,36))
   #define CLOSERET   void
   #define RWRET      long
   #define RWCOUNT_T  unsigned long
   #define RWINODE_P  struct inode *inode,
   #define RWPPOS_P
   #define RWINODE    inode
   #define KC_FOPS_FLUSH(ptr)
   #define kc_dev2minor MINOR
#elif (LINUX_VERSION_CODE < VERSION(2,1,76)) /* may need correction */
   #define CLOSERET   int
   #define RWRET      long
   #define RWCOUNT_T  unsigned long
   #define RWINODE_P  struct inode *inode,
   #define RWPPOS_P
   #define RWINODE    inode
   #define KC_FOPS_FLUSH(ptr)
   #define kc_dev2minor MINOR
#elif (LINUX_VERSION_CODE < VERSION(2,1,117)) /* may need correction */
   #define CLOSERET   int
   #define RWRET      ssize_t
   #define RWCOUNT_T  size_t
   #define RWINODE_P
   #define RWPPOS_P   ,loff_t *ppos
   #define RWINODE    file->f_dentry->d_inode
   #define KC_FOPS_FLUSH(ptr)
   #define kc_dev2minor MINOR
#elif (LINUX_VERSION_CODE < VERSION(2,5,7)) /* may need correction */
   #define CLOSERET   int
   #define RWRET      ssize_t
   #define RWCOUNT_T  size_t
   #define RWINODE_P
   #define RWPPOS_P   ,loff_t *ppos
   #define RWINODE    file->f_dentry->d_inode
   #define KC_FOPS_FLUSH(ptr)  flush:(ptr),
   #define kc_dev2minor MINOR
#else /* >= 2.5.7 */ /* may need correction */
   #define CLOSERET   int
   #define RWRET      ssize_t
   #define RWCOUNT_T  size_t
   #define RWINODE_P
   #define RWPPOS_P   ,loff_t *ppos
   #define RWINODE    file->f_dentry->d_inode
   #define KC_FOPS_FLUSH(ptr)  flush:(ptr),
   #define kc_dev2minor minor
#endif /* 2.1.36 */

#if (LINUX_VERSION_CODE < VERSION(2,1,50)) /* may need correction */
  #define kc_poll_wait(file,address,wait) poll_wait(address,wait)
#else /* >= 2.1.50 */
  #define kc_poll_wait poll_wait
#endif /* 2.1.50 */

/* definition of standard parameters for read/write file functions */
#define READ_PARAMETERS \
	RWINODE_P struct file *file, char *buf, RWCOUNT_T count RWPPOS_P

#define WRITE_PARAMETERS \
	RWINODE_P struct file *file, const char *buf, RWCOUNT_T count RWPPOS_P

#define LSEEK_PARAMETERS \
	RWINODE_P struct file *file, loff_t pos, int whence

#if (LINUX_VERSION_CODE < VERSION(2,2,0)) /* may need correction */
  #define KC_CHRDEV_FOPS_BEG(fops_var) \
        static struct file_operations fops_var =\
        {
  #define KC_CHRDEV_FOPS_END \
        }
  #define KC_FOPS_LSEEK(ptr)   lseek:(ptr),
  #define KC_FOPS_RELEASE(ptr) close:(ptr),
#elif (LINUX_VERSION_CODE < VERSION(2,4,0)) /* may need correction */
  #define KC_CHRDEV_FOPS_BEG(fops_var) \
        static struct file_operations fops_var =\
        {
  #define KC_CHRDEV_FOPS_END \
        }
  #define KC_FOPS_LSEEK(ptr)   llseek:(ptr),
  #define KC_FOPS_RELEASE(ptr) release:(ptr),
#else /* >= 2.4.0 */
  #define KC_CHRDEV_FOPS_BEG(fops_var) \
        static struct file_operations fops_var = {\
            owner:THIS_MODULE,
  #define KC_CHRDEV_FOPS_END \
        }
  #define KC_FOPS_LSEEK(ptr)   llseek:(ptr),
  #define KC_FOPS_RELEASE(ptr) release:(ptr),
#endif /* 2.4.0 */

/*** devices and drivers registration ***/

#if (LINUX_VERSION_CODE >= VERSION(2,5,41)) /* may need correction */
  #define devfs_register_chrdev register_chrdev
  #define devfs_unregister_chrdev unregister_chrdev
#endif /* 2.5.40 */

#if (LINUX_VERSION_CODE >= VERSION(2,5,60)) /* may need correction */
  #define kc_devfs_mk_dir devfs_mk_dir
#else /* 2.5.60 */
  #define kc_devfs_mk_dir(dirname...) ({ \
  	  char kc_buf[64]; int kc_n; \
	  n = snprintf(buf, 64, dirname...); \
  	  (n >= 64 || !buf[0])? NULL: devfs_mk_dir(NULL, kc_buf, NULL); \
	})

#endif /* 2.5.60 */

/*** tasklet declaration and processing ***/

#if (LINUX_VERSION_CODE < VERSION(2,5,0)) /* may need correction */
  #define kc_tasklet_struct tq_struct
  #define kc_tasklet_data_type void *
  #define KC_DECLARE_TASKLET(_name, _func, _data) \
  		struct tq_struct _name = { sync: 0, routine: _func, data: (void*)_data }
  		
  /* void tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data); */
  #define kc_tasklet_init(_tasklet, _func, _data) \
    do{ \
       /* (_tasklet)->next=NULL; */ \
       /* Above not needed for 2.2.x and buggy for 2.4.x */ \
       (_tasklet)->sync=0; \
       (_tasklet)->routine=_func; \
       (_tasklet)->data=(void*)_data; \
    }while(0)
    
  /* void tasklet_schedule(struct tasklet_struct *t) */
  #define kc_tasklet_schedule(_tasklet) \
    do{ \
       queue_task(_tasklet,&tq_immediate); \
       mark_bh(IMMEDIATE_BH); \
    }while(0)

  /* void tasklet_kill(struct tasklet_struct *t); */
  #define kc_tasklet_kill(_tasklet) \
  		synchronize_irq()

#else /* 2.5.40 */
  /* struct tasklet_struct */
  #define  kc_tasklet_struct tasklet_struct
  /* DECLARE_TASKLET(name, func, data) */
  #define kc_tasklet_data_type unsigned long
  #define KC_DECLARE_TASKLET DECLARE_TASKLET
  /* void tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data); */
  #define kc_tasklet_init tasklet_init
  /* void tasklet_schedule(struct tasklet_struct *t) */
  #define kc_tasklet_schedule tasklet_schedule
  /* void tasklet_kill(struct tasklet_struct *t); */
  #define kc_tasklet_kill tasklet_kill
#endif /* 2.5.40 */

/*** scheduler changes ***/

#if (LINUX_VERSION_CODE < VERSION(2,3,0)) /* may need correction */
  #define kc_yield schedule
#else /* 2.3.0 */
  #define kc_yield yield
#endif /* 2.3.0 */

/*** wait queues changes ***/

/* Next is not sctrictly correct, because of 2.3.0, 2.3.1, 2.3.2
   probably need next definitions */
#if (LINUX_VERSION_CODE < VERSION(2,2,19)) /* may need correction */
  #define wait_queue_head_t struct wait_queue *
  #define wait_queue_t      struct wait_queue
  #define init_waitqueue_head(queue_head) (*queue_head=NULL)
  #define init_waitqueue_entry(qentry,qtask) \
			(qentry->next=NULL,qentry->task=qtask) 
  #define DECLARE_WAIT_QUEUE_HEAD(name) \
   	struct wait_queue * name=NULL
  #define DECLARE_WAITQUEUE(wait, current) \
	struct wait_queue wait = { current, NULL }
  #define init_MUTEX(sem) (*sem=MUTEX)
  #define DECLARE_MUTEX(name) struct semaphore name=MUTEX
#endif /* 2.2.19 */

#ifndef MODULE_LICENSE
  #define MODULE_LICENSE(dummy)
#endif /* MODULE_LICENSE */

#endif /* _LINUX_K_COMPAT_H */
