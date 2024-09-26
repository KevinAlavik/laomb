#include <stdint.h>
#include <stddef.h>
#include <kprintf>
#include <io.h>
#include <string.h>

#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/pic.h>
#include <sys/mm/pmm.h>
#include <sys/mm/vmm.h>
#include <kheap.h>
#include <ultra_protocol.h>
#include <proc/task.h>
#include <proc/sched.h>

#include <rbtree.h>

uintptr_t higher_half_base;

struct ultra_platform_info_attribute* platform_info_attrb = NULL;
struct ultra_kernel_info_attribute* kernel_info_attrb = NULL;
struct ultra_memory_map_attribute* memory_map = NULL;
struct ultra_framebuffer_attribute* framebuffer = NULL;

void comprehensive_rbtree_test() {
    struct rb_tree *tree = create_tree();

    kprintf("===== Comprehensive RBTree Tests =====\n");

    kprintf("Test 1: Inserting multiple keys...\n");
    rb_insert(tree, 20);
    rb_insert(tree, 15);
    rb_insert(tree, 25);
    rb_insert(tree, 10);
    rb_insert(tree, 5);
    rb_insert(tree, 30);
    rb_inorder(tree->root);  // Expect: 5 10 15 20 25 30
    kprintf("\n");

    kprintf("Test 2: Searching for keys 15 and 100...\n");
    struct rb_node *node = rb_search(tree, 15);
    if (node != NULL) {
        kprintf("Node with key 15 found!\n");
    } else {
        kprintf("Node with key 15 not found!\n");
    }

    node = rb_search(tree, 100);
    if (node != NULL) {
        kprintf("Node with key 100 found! (Unexpected)\n");
    } else {
        kprintf("Node with key 100 not found! (Expected)\n");
    }

    kprintf("Test 3: Inserting duplicate key 15...\n");
    rb_insert(tree, 15);
    rb_inorder(tree->root);  // Expect: 5 10 15 20 25 30 (No change)
    kprintf("\n");

    kprintf("Test 4: Deleting leaf node (key 5)...\n");
    rb_delete(tree, 5);
    rb_inorder(tree->root);  // Expect: 10 15 20 25 30
    kprintf("\n");

    kprintf("Test 5: Deleting node with one child (key 10)...\n");
    rb_delete(tree, 10);
    rb_inorder(tree->root);  // Expect: 15 20 25 30
    kprintf("\n");

    kprintf("Test 6: Deleting node with two children (key 20)...\n");
    rb_delete(tree, 20);
    rb_inorder(tree->root);  // Expect: 15 25 30
    kprintf("\n");

    kprintf("Test 7: Deleting root node (key 15)...\n");
    rb_delete(tree, 15);
    rb_inorder(tree->root);  // Expect: 25 30
    kprintf("\n");

    kprintf("Test 8: Deleting from an empty tree...\n");
    rb_delete(tree, 25);
    rb_delete(tree, 30);  // Tree should now be empty
    rb_inorder(tree->root);  // Expect: No output
    kprintf("\n");

    kprintf("Test 9: Deleting non-existent node (key 50)...\n");
    rb_delete(tree, 50);
    rb_inorder(tree->root);
    kprintf("\n");

    kprintf("Test 10: Inserting new keys into empty tree...\n");
    rb_insert(tree, 40);
    rb_insert(tree, 35);
    rb_insert(tree, 45);
    rb_inorder(tree->root);  // Expect: 35 40 45
    kprintf("\n");

    kprintf("Test 11: Deleting root node with only one child (key 40)...\n");
    rb_delete(tree, 40);
    rb_inorder(tree->root);  // Expect: 35 45
    kprintf("\n");

    kprintf("Test 12: Deleting single node in tree (key 35 and key 45)...\n");
    rb_delete(tree, 35);
    rb_delete(tree, 45);
    rb_inorder(tree->root);
    kprintf("\n");

    kprintf("Test 13: Inserting large number of elements...\n");
    for (int i = 1; i <= 100; i++) {
        rb_insert(tree, i);
    }
    rb_inorder(tree->root);
    kprintf("\n");

    kprintf("Test 14: Deleting all elements in reverse order...\n");
    for (int i = 100; i >= 1; i--) {
        rb_delete(tree, i);
    }
    rb_inorder(tree->root);
    kprintf("\n");

    kprintf("All tests completed.\n");
}

void main() {
    comprehensive_rbtree_test();

    for(;;) hlt();
}

[[noreturn]] void _start(struct ultra_boot_context* ctx, uint32_t)
{
    cli();

    higher_half_base = (uintptr_t)(((struct ultra_platform_info_attribute*)ctx->attributes)->higher_half_base);

    gdt_init();
    idt_init();
    pmm_init(ctx);

    vmm_init_pd(&kernel_page_directory);
    vmm_switch_pd(&kernel_page_directory);

    struct ultra_attribute_header* head = ctx->attributes;

    for (size_t i = 0; i < ctx->attribute_count; i++, head = ULTRA_NEXT_ATTRIBUTE(head)) {
        switch (head->type) {
            case ULTRA_ATTRIBUTE_PLATFORM_INFO:
                platform_info_attrb = kmalloc(head->size);
                memcpy(platform_info_attrb, head, head->size);
                break;
            case ULTRA_ATTRIBUTE_KERNEL_INFO:
                kernel_info_attrb = kmalloc(head->size);
                memcpy(kernel_info_attrb, head, head->size);
                break;
            case ULTRA_ATTRIBUTE_MEMORY_MAP:
                memory_map = kmalloc(head->size);
                memcpy(memory_map, head, head->size);
                break;
            case ULTRA_ATTRIBUTE_FRAMEBUFFER_INFO:
                framebuffer = kmalloc(head->size);
                memcpy(framebuffer, head, head->size);
                break;
            default:
                break;
        }
    }
    pmm_memory_map = memory_map;
    pmm_reclaim_bootloader_memory();

    struct task callback_task = task_create((uintptr_t)main, 0, 0, 20, kernel_page_directory);
    sched_init(&callback_task);

    for(;;) ;
}