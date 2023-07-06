// Copyright 2019-2022 The jdh99 Authors. All rights reserved.
// 重复帧检测模块
// duplicate frame简称df
// Authors: jdh99 <jdh821@163.com>

#include "dfdetector.h"
#include "tzmalloc.h"
#include "async.h"
#include "tzlist.h"
#include "tztime.h"
#include "lagan.h"

#define TAG "dftector"
#define MALLOC_TOTAL 2048

#pragma pack(1)

// tUnit 存储重复帧信息的数据结构
typedef struct {
    uint32_t id;
    uint32_t index;
    uint64_t time;
} tUnit;

#pragma pack()

// tzmalloc用户id
static int gMid = -1;
// 存储帧序号的列表
static intptr_t gList = 0;

static uint32_t gExpirationTime = 0;
static uint32_t gDeltaIndexMax = 0;

static int checkExpireTask(void);
static uint32_t absMy(uint32_t num1, uint32_t num2);

// removeAllExpirationNode 删除从首节点到startNode节点的所有过期节点
static void removeAllExpirationNode(TZListNode* startNode);

TZListNode* createNode(uint32_t id, uint32_t index);

// DFDetectorLoad 模块载入
// expirationTime 过期时间.单位:ms
// deltaIndexMax 最大不同的序号.如果序号是1个字节,建议是250,如果是2个字节建议是65000
bool DFDetectorLoad(int expirationTime, uint32_t deltaIndexMax) {
    gExpirationTime = expirationTime;
    gDeltaIndexMax = deltaIndexMax;

    gMid = TZMallocRegister(0, TAG, MALLOC_TOTAL);
    gList = TZListCreateList(gMid);

    if (gMid == -1 || gList == 0) {
        LE(TAG, "load failed.malloc failed");
        return false;
    }

    if (AsyncStart(checkExpireTask, TZTIME_SECOND) == false) {
        LE(TAG, "load failed.async start failed");
        return false;
    }
    return true;
}

static int checkExpireTask(void) {
    static struct pt pt = {0};
    static TZListNode* node = NULL;
    static uint64_t time = 0;
    static tUnit* unit;
    
    PT_BEGIN(&pt);

    time = TZTimeGetMillsecond();
    for (;;) {
        node = TZListGetHeader(gList);
        if (node == NULL) {
            break;
        }

        unit = (tUnit*)node->Data;
        if (time - unit->time <= gExpirationTime) {
            break;
        }

        TZListRemove(gList, node);
        PT_YIELD(&pt);
    }
    
    PT_END(&pt);
}

// DFDetectorQuery 查询是否有重复帧
// 查询时会清除过期节点
bool DFDetectorQuery(uint32_t id, uint32_t index) {
    TZListNode* node = TZListGetTail(gList);
    if (node == NULL) {
        return false;
    }

    TZListNode* nodeLast = NULL;
    uint64_t timeNow = TZTimeGetMillsecond();
    tUnit* unit;
    for (;;) {
        nodeLast = node->Last;
        
        unit = (tUnit*)node->Data;
        // 有过期节点则前面所有节点都是过期节点
        if (timeNow - unit->time > gExpirationTime) {
            removeAllExpirationNode(node);
            return false;
        }

        if (unit->id == id && unit->index == index) {
            return true;
        }
        
        if (unit->id == id && absMy(unit->index, index) > gDeltaIndexMax) {
            TZListRemove(gList, node);
        }
        
        node = nodeLast;
        if (node == NULL) {
            return false;
        }
    }
}

static uint32_t absMy(uint32_t num1, uint32_t num2) {
    if (num1 > num2) {
        return num1 - num2;
    } else {
        return num2 - num1;
    }
}

// removeAllExpirationNode 删除从首节点到startNode节点的所有过期节点
static void removeAllExpirationNode(TZListNode* startNode) {
    TZListNode* node = NULL;
    for (;;) {
        node = TZListGetHeader(gList);
        if (node == NULL) {
            return;
        }
        TZListRemove(gList, node);
        if (node == startNode) {
            return;
        }
    }
}

// DFDetectorInsert 插入帧信息.如果是重复帧,则插入失败
bool DFDetectorInsert(uint32_t id, uint32_t index) {
    if (DFDetectorQuery(id, index) == true) {
        return false;
    }

    TZListNode* node;
    for (;;) {
        node = createNode(id, index);
        if (node != NULL) {
            break;
        }
        // 如果链表是空的都不能插入,则插入失败返回,否则删除最旧节点
        if (TZListIsEmpty(gList) == true) {
            LE(TAG, "insert fail!list is error!");
            return false;
        }
        TZListRemove(gList, TZListGetHeader(gList));
    }
    TZListAppend(gList, node);
    return true;
}

TZListNode* createNode(uint32_t id, uint32_t index) {
    TZListNode* node = TZListCreateNode(gList);
    if (node == NULL) {
        return NULL;
    }
    node->Data = TZMalloc(gMid, sizeof(tUnit));
    if (node->Data == NULL) {
        TZFree(node);
        return NULL;
    }
    node->Size = sizeof(tUnit);
    tUnit* unit = (tUnit*)node->Data;
    unit->id = id;
    unit->index = index;
    unit->time = TZTimeGetMillsecond();

    return node;
}

// DFDetectorRemove 移除帧信息
void DFDetectorRemove(uint32_t id, uint32_t index) {
    TZListNode* node = TZListGetHeader(gList);
    if (node == NULL) {
        return;
    }

    tUnit* unit;
    for (;;) {
        unit = (tUnit*)node->Data;
        if (unit->id == id && unit->index == index) {
            TZListRemove(gList, node);
            return;
        }

        node = node->Next;
        if (node == NULL) {
            return;
        }
    }
}

// DFDetectorClear 清除所有帧信息
void DFDetectorClear(void) {
    TZListClear(gList);
}
