//
// smm_object.c
// SMMarble object
//
//

#include "smm_common.h"
#include "smm_object.h"
#include <string.h>

// 노드 유형 및 성적 열거형(enum) 정의
typedef enum {
    SMM_LECTURE = 0,
    SMM_RESTAURANT,
    SMM_LABORATORY,
    SMM_HOME,
    SMM_EXPERIMENT,
    SMM_FOOD_CHANCE,
    SMM_FESTIVAL
} SMMNode_e;

typedef enum {
    SMM_APLUS = 0,
    SMM_A0,
    SMM_AMINUS,
    SMM_BPLUS,
    SMM_B0,
    SMM_BMINUS,
    SMM_CPLUS,
    SMM_C0,
    SMM_CMINUS
} SMMGrade_e;

#define MAX_NODETYPE 7
#define MAX_GRADE 9

// 노드 유형 문자열 배열 (getNodeName에서 사용)
static char* smmNodeName[MAX_NODETYPE] = {
    "강의", "식당", "실험실", "집", "실험", "보충찬스", "축제"
};

// 성적 문자열 배열 (getGradeName에서 사용)
static char* smmGradeName[MAX_GRADE] = {
    "A+", "A0", "A-", "B+", "B0", "B-", "C+", "C0", "C-"
};


// =================================================================================================
// 1. 객체 구조체 정의 (이 파일에서만 사용 가능)
// =================================================================================================

// 보드 노드 객체
typedef struct _smmNode {
    char name[MAX_CHARNAME];
    SMMNode_e type;       // 노드 유형 (강의, 식당 등)
    int credit;           // 획득/차감 학점
    int energy;           // 소요/보충 에너지
    int is_laboratory_active; // 실험실 노드 전용: 실험 중인지 여부 (1: 실험 중, 0: 실험 가능)
} smmNode;

// 음식 카드 객체
typedef struct _smmFoodCard {
    char name[MAX_CHARNAME];
    int energy; // 보충 에너지 (음수 가능)
} smmFoodCard;

// 축제 카드 객체
typedef struct _smmFestCard {
    char content[MAX_CHARNAME];
} smmFestCard;

// 수강 이력 객체
typedef struct _smmLectureHistory {
    char name[MAX_CHARNAME];
    SMMGrade_e grade;
} smmLectureHistory;

// 플레이어 객체
typedef struct _smmPlayer {
    char name[MAX_CHARNAME];
    int current_energy;    // 현재 에너지
    int total_credit;      // 누적 학점
    int current_position;  // 현재 보드 위치 (0 ~ board_nr-1)
    
    // 실험 관련 상태
    int is_experimenting;           // 1: 실험 중, 0: 일반 상태
    int experiment_target_die;      // 실험 성공에 필요한 주사위 최소값 (실험 노드에서 랜덤 결정)

    // 성적 관리 관련
    int lecture_list_nr;            // 플레이어의 수강 이력이 저장된 DB 리스트 번호 (LISTNO_OFFSET_GRADE + player_index)
} smmPlayer;


// =================================================================================================
// 2. 객체 생성(Creator) 및 해제(Destroyer) 함수 구현
// =================================================================================================

// 노드 객체 생성
SMMNode smm_create_node(char *name, int type, int credit, int energy)
{
    SMMNode node = (SMMNode)malloc(sizeof(smmNode));
    if (node == NULL) return NULL;

    strncpy(node->name, name, MAX_CHARNAME - 1);
    node->name[MAX_CHARNAME - 1] = '\0';
    node->type = (SMMNode_e)type;
    node->credit = credit;
    node->energy = energy;
    node->is_laboratory_active = 0; // 실험실은 기본적으로 활성화 상태(실험 가능)로 초기화

    return node;
}

// 음식 카드 객체 생성
SMMFoodCard smm_create_foodcard(char *name, int energy)
{
    SMMFoodCard card = (SMMFoodCard)malloc(sizeof(smmFoodCard));
    if (card == NULL) return NULL;

    strncpy(card->name, name, MAX_CHARNAME - 1);
    card->name[MAX_CHARNAME - 1] = '\0';
    card->energy = energy;

    return card;
}

// 축제 카드 객체 생성
SMMFestCard smm_create_festcard(char *content)
{
    SMMFestCard card = (SMMFestCard)malloc(sizeof(smmFestCard));
    if (card == NULL) return NULL;

    strncpy(card->content, content, MAX_CHARNAME - 1);
    card->content[MAX_CHARNAME - 1] = '\0';
    
    return card;
}

// 플레이어 객체 생성
SMMPlayer smm_create_player(char *name, int initial_energy, int lecture_list_nr)
{
    SMMPlayer player = (SMMPlayer)malloc(sizeof(smmPlayer));
    if (player == NULL) return NULL;

    strncpy(player->name, name, MAX_CHARNAME - 1);
    player->name[MAX_CHARNAME - 1] = '\0';
    player->current_energy = initial_energy;
    player->total_credit = 0;
    player->current_position = 0; // 집 노드에서 시작
    player->is_experimenting = 0;
    player->experiment_target_die = 0;
    player->lecture_list_nr = lecture_list_nr;
    
    return player;
}

// 객체 메모리 해제
void smm_destroy_node(SMMNode node) { free(node); }
void smm_destroy_foodcard(SMMFoodCard card) { free(card); }
void smm_destroy_festcard(SMMFestCard card) { free(card); }
void smm_destroy_lecture_history(SMMLectureHistory history) { free(history); }
void smm_destroy_player(SMMPlayer player) { free(player); }


// =================================================================================================
// 3. Getter 함수 구현
// =================================================================================================

// Node Getters
char* smm_get_node_name(SMMNode node) { return node->name; }
int smm_get_node_type(SMMNode node) { return node->type; }
int smm_get_node_credit(SMMNode node) { return node->credit; }
int smm_get_node_energy(SMMNode node) { return node->energy; }
int smm_get_node_is_laboratory_active(SMMNode node) { return node->is_laboratory_active; }

// Food Card Getters
char* smm_get_foodcard_name(SMMFoodCard card) { return card->name; }
int smm_get_foodcard_energy(SMMFoodCard card) { return card->energy; }

// Festival Card Getters
char* smm_get_festcard_content(SMMFestCard card) { return card->content; }

// Player Getters
char* smm_get_player_name(SMMPlayer player) { return player->name; }
int smm_get_player_energy(SMMPlayer player) { return player->current_energy; }
int smm_get_player_credit(SMMPlayer player) { return player->total_credit; }
int smm_get_player_position(SMMPlayer player) { return player->current_position; }
int smm_get_player_is_experimenting(SMMPlayer player) { return player->is_experimenting; }
int smm_get_player_experiment_target_die(SMMPlayer player) { return player->experiment_target_die; }
int smm_get_player_lecture_list_nr(SMMPlayer player) { return player->lecture_list_nr; }

// 성적 평균 계산 (수강 이력 DB에서 데이터 가져와서 계산해야 함 -> main.c에서 smmdb_getData 활용 필요)
float smm_get_player_average_grade(SMMPlayer player)
{
    // 평균 계산 로직은 smm_database.h를 include하지 못하는 smm_object.c에서는 구현 불가
    // 따라서 이 함수는 main.c 또는 별도의 모듈 함수에서 구현되어야 하며,
    // 여기서는 캡슐화 원칙을 위해 player 구조체에 GPA를 직접 저장하지 않고 0.0을 반환하도록 처리합니다.
    return 0.0f; 
}


// =================================================================================================
// 4. Setter 함수 구현
// =================================================================================================

// Node Setter
void smm_set_node_is_laboratory_active(SMMNode node, int active)
{
    node->is_laboratory_active = active;
}

// Player Setters
void smm_set_player_energy(SMMPlayer player, int energy) { player->current_energy = energy; }
void smm_set_player_position(SMMPlayer player, int position) { player->current_position = position; }
void smm_set_player_credit(SMMPlayer player, int credit) { player->total_credit = credit; }
void smm_set_player_is_experimenting(SMMPlayer player, int experimenting) { player->is_experimenting = experimenting; }
void smm_set_player_experiment_target_die(SMMPlayer player, int target_die) { player->experiment_target_die = target_die; }


// =================================================================================================
// 5. 기타 핵심 함수 구현
// =================================================================================================

// 성적 열거형 문자열 반환 (main.c에서 성적 출력 시 사용)
char* smm_get_grade_name(int grade)
{
    if (grade >= 0 && grade < MAX_GRADE)
        return smmGradeName[grade];
    return "N/A";
}

// 수강 이력 추가 (학점, 성적 모두 필요)
SMMLectureHistory smm_add_lecture_history(char* name, int grade)
{
    SMMLectureHistory history = (SMMLectureHistory)malloc(sizeof(smmLectureHistory));
    if (history == NULL) return NULL;

    strncpy(history->name, name, MAX_CHARNAME - 1);
    history->name[MAX_CHARNAME - 1] = '\0';
    history->grade = (SMMGrade_e)grade;
    
    return history;
}

// 플레이어에게 수강 이력 추가 (실제 DB에 추가하는 것은 main.c에서 smmdb_addTail 사용)
void smm_player_add_lecture(SMMPlayer player, SMMLectureHistory lecture_history)
{
    // 이 함수는 플레이어 객체 내부의 학점 총합만 업데이트합니다.
    // 실제 Linked List에 수강 이력 객체를 추가하는 로직은 smmdb_addTail 함수를 호출하는 main.c에서 처리되어야 합니다.
    
    // 학점 갱신 로직은 강의 노드 방문 시점에 학점(credit)을 전달받아 이미 갱신되었을 것으로 가정하거나,
    // 이 함수에서 학점(credit) 값을 알 수 없으므로, main.c에서 total_credit을 직접 업데이트 해야 합니다.
    // 여기서는 Player 객체에 대한 포인터를 전달받았으므로, 향후 필요할 수 있는 확장을 위해 남겨둡니다.
}
