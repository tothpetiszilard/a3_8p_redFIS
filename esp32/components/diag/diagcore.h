#ifndef DIAGCORE_H_
#define DIAGCORE_H_

#include "stdint.h"

#define DIAG_OK       (0u)
#define DIAG_ERR      (1u)
#define DIAG_PENDING  (2u)

typedef uint8_t Diag_ReturnType;
typedef void (Diag_CallbackType)(uint8_t *, uint8_t);

extern Diag_ReturnType Diag_Init(uint8_t ecuId, Diag_CallbackType *cb);
extern Diag_ReturnType Diag_DeInit(void);
extern void Diag_Cyclic(void *pvParameters);
extern Diag_ReturnType Diag_ReqDid(uint8_t did);

#endif // DIAGCORE_H_