//
// smm_object.h
// SMMarble object
//
//

#ifndef smm_object_h
#define smm_object_h

#include "smm_common.h"

// 불투명 포인터 타입 선언 (Data Encapsulation 핵심)
typedef struct _smmNode *SMMNode;
typedef struct _smmFoodCard *SMMFoodCard;
typedef struct _smmFestCard *SMMFestCard;
typedef struct _smmLectureHistory *SMMLectureHistory;
typedef struct _smmPlayer *SMMPlayer;


// =================================================================================================
// 객체 생성(Creator) 및 해제(Destroyer) 함수
// =================================================================================================

// 노드 객체 생성
SMMNode smm_create_node(char *name, int type, int credit, int energy);
// 음식 카드 객체 생성
SMMFoodCard smm_create_foodcard(char *name, int energy);
// 축제 카드 객체 생성
SMMFestCard smm_create_festcard(char *content);
// 플레이어 객체 생성
SMMPlayer smm_create_player(char *name, int initial_energy, int lecture_list_nr);

// 객체 메모리 해제
void smm_destroy_node(SMMNode node);
void smm_destroy_foodcard(SMMFoodCard card);
void smm_destroy_festcard(SMMFestCard card);
void smm_destroy_lecture_history(SMMLectureHistory history);
void smm_destroy_player(SMMPlayer player);


// =================================================================================================
// Getter 함수 (객체 정보 반환)
// =================================================================================================

// Node Getters
char* smm_get_node_name(SMMNode node);
int smm_get_node_type(SMMNode node);
int smm_get_node_credit(SMMNode node);
int smm_get_node_energy(SMMNode node);
int smm_get_node_is_laboratory_active(SMMNode node);

// Food Card Getters
char* smm_get_foodcard_name(SMMFoodCard card);
int smm_get_foodcard_energy(SMMFoodCard card);

// Festival Card Getters
char* smm_get_festcard_content(SMMFestCard card);

// Player Getters
char* smm_get_player_name(SMMPlayer player);
int smm_get_player_energy(SMMPlayer player);
int smm_get_player_credit(SMMPlayer player);
int smm_get_player_position(SMMPlayer player);
int smm_get_player_is_experimenting(SMMPlayer player);
int smm_get_player_experiment_target_die(SMMPlayer player);
int smm_get_player_lecture_list_nr(SMMPlayer player);
float smm_get_player_average_grade(SMMPlayer player);


// =================================================================================================
// Setter 함수 (객체 정보 변경)
// =================================================================================================

// Node Setter (실험실 활성화 상태 변경)
void smm_set_node_is_laboratory_active(SMMNode node, int active);

// Player Setters
void smm_set_player_energy(SMMPlayer player, int energy);
void smm_set_player_position(SMMPlayer player, int position);
void smm_set_player_credit(SMMPlayer player, int credit);
void smm_set_player_is_experimenting(SMMPlayer player, int experimenting);
void smm_set_player_experiment_target_die(SMMPlayer player, int target_die);

// =================================================================================================
// 기타 핵심 함수
// =================================================================================================

// 성적 열거형(enum) 정의에 사용될 이름 문자열 반환 (smm_object.c에서 사용)
char* smm_get_grade_name(int grade);
// 수강 이력 추가
SMMLectureHistory smm_add_lecture_history(char* name, int grade);
// 플레이어에게 수강 이력 추가
void smm_player_add_lecture(SMMPlayer player, SMMLectureHistory lecture_history);


#endif /* smm_object_h */
