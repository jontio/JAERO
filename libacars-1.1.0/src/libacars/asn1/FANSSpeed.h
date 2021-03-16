/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "FANSACTwoWayDataLinkCommunications"
 * 	found in "fans-cpdlc.asn1"
 * 	`asn1c -fcompound-names -fincludes-quoted -gen-PER`
 */

#ifndef	_FANSSpeed_H_
#define	_FANSSpeed_H_


#include "asn_application.h"

/* Including external dependencies */
#include "FANSSpeedIndicated.h"
#include "FANSSpeedIndicatedMetric.h"
#include "FANSSpeedTrue.h"
#include "FANSSpeedTrueMetric.h"
#include "FANSSpeedGround.h"
#include "FANSSpeedGroundMetric.h"
#include "FANSSpeedMach.h"
#include "FANSSpeedMachLarge.h"
#include "constr_CHOICE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum FANSSpeed_PR {
	FANSSpeed_PR_NOTHING,	/* No components present */
	FANSSpeed_PR_speedIndicated,
	FANSSpeed_PR_speedIndicatedMetric,
	FANSSpeed_PR_speedTrue,
	FANSSpeed_PR_speedTrueMetric,
	FANSSpeed_PR_speedGround,
	FANSSpeed_PR_speedGroundMetric,
	FANSSpeed_PR_speedMach,
	FANSSpeed_PR_speedMachLarge
} FANSSpeed_PR;

/* FANSSpeed */
typedef struct FANSSpeed {
	FANSSpeed_PR present;
	union FANSSpeed_u {
		FANSSpeedIndicated_t	 speedIndicated;
		FANSSpeedIndicatedMetric_t	 speedIndicatedMetric;
		FANSSpeedTrue_t	 speedTrue;
		FANSSpeedTrueMetric_t	 speedTrueMetric;
		FANSSpeedGround_t	 speedGround;
		FANSSpeedGroundMetric_t	 speedGroundMetric;
		FANSSpeedMach_t	 speedMach;
		FANSSpeedMachLarge_t	 speedMachLarge;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} FANSSpeed_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_FANSSpeed;

#ifdef __cplusplus
}
#endif

#endif	/* _FANSSpeed_H_ */
#include "asn_internal.h"