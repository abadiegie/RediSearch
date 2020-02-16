#ifndef RULES_RULES_H
#define RULES_RULES_H

#include "redismodule.h"
#include "rmutil/args.h"
#include "query_error.h"
#include "util/dllist.h"

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SchemaRules SchemaRules;
typedef struct SchemaRule SchemaRule;

// Create the rule list. This is usually global..
SchemaRules *SchemaRules_Create(void);
void SchemaRules_Free(SchemaRules *rules);

/**
 * Add rules pertaining to an index
 * @param rules
 * @param index the index name. If the rule matches, it will be dispatched to
 *              the index
 * @param name a globally unique name (across all indexes) for the rule. This
 *        can be used to further identify the rule
 * @param ac the arguments passed
 * @param[out] err if there is an error
 * @return REDISMODULE_OK/REDISMODULE_ERR
 *
 * Note, this function consumes only a single rule.
 */
int SchemaRules_AddArgs(SchemaRules *rules, const char *index, const char *name, ArgsCursor *ac,
                        QueryError *err);

typedef struct {
  char *language; // can be an enum??
  void *payload;
  size_t npayload;
  float score;
} IndexItemAttrs;

typedef struct {
  const char *index;
  IndexItemAttrs attrs;
} MatchAction;

typedef struct {
  RedisModuleString *kstr;
  RedisModuleKey *kobj;     // Necessary? We are appending to the currect idx.
} RuleKeyItem;

typedef struct asyncIndexCtx {
  RedisModuleCtx *aiCtx;

  DLLIST indexList;         // List of indexes with documents to be indexed

  size_t interval;          // interval in milliseconds. sleep time when queue is empty
  size_t indexBatchSize;    // maximum documents to index at once. Prevents starvation   

  pthread_t aiThread;
  pthread_mutex_t lock;
} asyncIndexCtx;

typedef struct RuleIndexableDocument {
  DLLIST_node llnode;
  RuleKeyItem rki;
  IndexItemAttrs iia;
} RuleIndexableDocument;



// Check if the given document matches any of the rule sets
int SchemaRules_Check(const SchemaRules *rules, RedisModuleCtx *ctx, RuleKeyItem *item,
                      MatchAction **results, size_t *nresults);

extern SchemaRules *SchemaRules_g;

/**
 * Submits all the keys in the database for indexing
 */
void SchemaRules_ScanAll(const SchemaRules *rules);

/**
 * Initializes the global rule list and subscribes to keyspace events
 */
void SchemaRules_InitGlobal();
void SchemaRules_ShutdownGlobal();

#ifdef __cplusplus
}
#endif
#endif