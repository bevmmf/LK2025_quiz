#include <stdio.h>
#include <stdlib.h>

// block_t 結構體定義
typedef struct block {
    size_t size;
    struct block *l; // 左子節點
    struct block *r; // 右子節點
} block_t;

// 查找目標節點
block_t **find_free_tree(block_t **root, block_t *target) {
    block_t **current = root;
    while (*current != NULL) {
        if (target->size < (*current)->size) {
            current = &(*current)->l;
        } else if (target->size > (*current)->size) {
            current = &(*current)->r;
        } else {
            if (*current == target) {
                return current;
            } else {
                return NULL;
            }
        }
    }
    return NULL;
}

// 查找中序前驅
block_t *find_predecessor_free_tree(block_t **root, block_t *node) {
    if (node == NULL || node->l == NULL) {
        return NULL;
    }
    block_t *pred = node->l;
    while (pred->r != NULL) {
        pred = pred->r;
    }
    return pred;
}

// 移除節點（簡單實現）
void remove_free_tree(block_t **root, block_t *target) {
    block_t **node_ptr = find_free_tree(root, target);
    if (node_ptr == NULL) return; // 未找到目標節點

    block_t *node = *node_ptr;
    // 情況 1：無子節點或只有一個子節點
    if (node->l == NULL) {
        *node_ptr = node->r;
    } else if (node->r == NULL) {
        *node_ptr = node->l;
    } else {
        // 情況 2：有兩個子節點
        block_t **pred_ptr = &node->l;
        while ((*pred_ptr)->r) {
            pred_ptr = &(*pred_ptr)->r; // 找到中序前驅
        }
        block_t *pred = *pred_ptr;
        *pred_ptr = pred->l; // 用前驅的左子樹替換前驅位置
        pred->l = node->l;   // 前驅接管 node 的左子樹
        pred->r = node->r;   // 前驅接管 node 的右子樹
        *node_ptr = pred;    // 用前驅替換 node
    }
}

// 輔助函數：插入節點
void insert_free_tree(block_t **root, block_t *node) {
    if (*root == NULL) {
        *root = node;
        node->l = NULL;
        node->r = NULL;
    } else if (node->size < (*root)->size) {
        insert_free_tree(&(*root)->l, node);
    } else {
        insert_free_tree(&(*root)->r, node);
    }
}

// 輔助函數：中序遍歷打印樹
void inorder_print(block_t *root) {
    if (root != NULL) {
        inorder_print(root->l);
        printf("%zu ", root->size);
        inorder_print(root->r);
    }
}

// 測試 main 函數
int main() {
    // 創建節點
    block_t *node1 = malloc(sizeof(block_t)); node1->size = 10;
    block_t *node2 = malloc(sizeof(block_t)); node2->size = 5;
    block_t *node3 = malloc(sizeof(block_t)); node3->size = 15;
    block_t *node4 = malloc(sizeof(block_t)); node4->size = 3;
    block_t *node5 = malloc(sizeof(block_t)); node5->size = 7;

    // 構建 BST
    block_t *root = NULL;
    insert_free_tree(&root, node1);
    insert_free_tree(&root, node2);
    insert_free_tree(&root, node3);
    insert_free_tree(&root, node4);
    insert_free_tree(&root, node5);

    // 打印原始樹
    printf("Original tree (in-order): ");
    inorder_print(root);
    printf("\n");

    // 移除 size=10 的節點
    remove_free_tree(&root, node1);
    printf("Tree after removing size 10: ");
    inorder_print(root);
    printf("\n");

    // 清理記憶體（簡化處理）
    free(node1);
    free(node2);
    free(node3);
    free(node4);
    free(node5);

    return 0;
}
