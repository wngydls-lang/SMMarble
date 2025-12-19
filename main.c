//
//  main.c
//  SMMarble
//
//

#include <stdio.h> 
#include <time.h>
#include <string.h>
#include <stdlib.h> 

#include "smm_object.h"
#include "smm_database.h"
#include "smm_common.h"

#define BOARDFILEPATH "marbleBoardConfig.txt"
#define FOODFILEPATH "marbleFoodConfig.txt"
#define FESTFILEPATH "marbleFestivalConfig.txt"


//board configuration parameters
static int board_nr;
static int food_nr;
static int festival_nr;


// ========================================================================================
// 강의 성적 학점 
 
typedef enum smmGrade_e {
    SMM_APLUS, SMM_A0, SMM_AMINUS,
    SMM_BPLUS, SMM_B0, SMM_BMINUS,
    SMM_CPLUS, SMM_C0, SMM_CMINUS,
} SMMGrade_e;


// ========================================================================================
// 모든 함수의 원형 선언 (Prototype)

int isGraduated(SMMPlayer *players, int player_nr, int board_nr);
void printPlayerStatus(SMMPlayer *players, int player_nr, int board_nr);
void goForward(int player_index, int step, SMMPlayer *players, int board_nr);
void actionNode(int player_index, SMMPlayer *players, int board_nr, int die_result);
int rolldie(int player_index, SMMPlayer *players);
float calcAverageGrade(int player_index, SMMPlayer *players);
SMMGrade_e takeLecture(int player_index, char *lectureName, int credit, SMMPlayer *players);
void printGrades(int player_index, SMMPlayer *players);
void printFinalResult(SMMPlayer *players, int player_nr);
int hasAlreadyTaken(int player_index, char *lectureName, SMMPlayer *players);


// =================================================================================================
// 함수 구현 시작
// =================================================================================================

// 중복 수강 체크 함수 (새로 추가)
int hasAlreadyTaken(int player_index, char *lectureName, SMMPlayer *players)
{
    SMMPlayer p;
    int list_nr;
    int total_lectures;
    int i;
    SMMLectureHistory history;
    char* existing_lecture_name;
    
    p = players[player_index];
    list_nr = smm_get_player_lecture_list_nr(p);
    total_lectures = smmdb_listCount(list_nr);
    
    for (i = 0; i < total_lectures; i++)
    {
        history = (SMMLectureHistory)smmdb_getData(list_nr, i);
        existing_lecture_name = smm_get_lecture_name(history);
        
        if (strcmp(existing_lecture_name, lectureName) == 0)
        {
            return 1; // 이미 수강한 강의
        }
    }
    
    return 0; // 수강하지 않은 강의
}

// check if any player is graduated
// 졸업 조건: GRADUATE_CREDIT 이상 이수 + 집 노드(유형 3)에 위치
int isGraduated(SMMPlayer *players, int player_nr, int board_nr)
{
    int i; 

    for (i = 0; i < player_nr; i++)
    {
        // 1. 학점 확인: GRADUATE_CREDIT 이상
        if (smm_get_player_credit(players[i]) >= GRADUATE_CREDIT)
        {
            // 2. 위치 확인: 집 노드(0번 위치)에 있는지
            if (smm_get_player_position(players[i]) == 0)
            {
                printf("\n=======================================================\n");
                printf("  [★ GRADUATED!] %s가 졸업하여 게임을 종료합니다! ", smm_get_player_name(players[i]));
                printf("\n=======================================================\n");
                return 1; // 졸업한 플레이어가 있으면 1 반환 (게임 종료)
            }
        }
    }
    return 0; // 졸업한 플레이어가 없으면 0 반환
}

// print all player status at the beginning of each turn
void printPlayerStatus(SMMPlayer *players, int player_nr, int board_nr)
{
    int i;
    SMMPlayer p;
    int pos;
    SMMNode current_node;
    float gpa;
    
    printf("\n\n-------------------------------------------------------\n");
    printf("              CURRENT PLAYER STATUS                  \n");
    printf("-------------------------------------------------------\n");
    
    for (i = 0; i < player_nr; i++)
    {
        p = players[i];
        pos = smm_get_player_position(p);
        current_node = (SMMNode)smmdb_getData(LISTNO_NODE, pos);
        
        gpa = calcAverageGrade(i, players);
        
        printf("[P%i] %s (E: %i, C: %i, GPA: %.2f) | Location: %i(%s)",
               i + 1,
               smm_get_player_name(p),
               smm_get_player_energy(p),
               smm_get_player_credit(p),
               gpa,
               pos,
               smm_get_node_name(current_node));

        if (smm_get_player_is_experimenting(p))
        {
            printf(" [EXPERIMENTING: Target %i]", smm_get_player_experiment_target_die(p));
        }
        
        printf("\n");
    }
    printf("-------------------------------------------------------\n");
}

// make player go "step" steps on the board
void goForward(int player_index, int step, SMMPlayer *players, int board_nr)
{
    SMMPlayer p;
    int current_pos;
    int current_energy;
    SMMNode home_node;
    int home_energy;
    int i;
    int next_pos;
    SMMNode next_node;
    SMMNode final_node; 
    
    p = players[player_index];
    current_pos = smm_get_player_position(p);
    current_energy = smm_get_player_energy(p);
    
    home_node = (SMMNode)smmdb_getData(LISTNO_NODE, 0);
    home_energy = smm_get_node_energy(home_node);

    printf(">> %s (P%i)가 %i칸 이동합니다. 현재 위치: %i\n", 
           smm_get_player_name(p), player_index + 1, step, current_pos);

    for (i = 1; i <= step; i++)
    {
        next_pos = (current_pos + 1) % board_nr;
        next_node = (SMMNode)smmdb_getData(LISTNO_NODE, next_pos);

        if (next_pos == 0)
        {
            current_energy += home_energy;
            smm_set_player_energy(p, current_energy);
            printf("  [PASS HOME] %s(%i) 노드 통과! 에너지를 %i 보충 받습니다. (현재 에너지: %i)\n", 
                   smm_get_node_name(next_node), next_pos, home_energy, current_energy);
        }

        current_pos = next_pos;
    }
    
    smm_set_player_position(p, current_pos);
    final_node = (SMMNode)smmdb_getData(LISTNO_NODE, current_pos);
    printf(">> 최종 도착 위치: %i(%s) \n", current_pos, smm_get_node_name(final_node));
}

// action code when a player stays at a node
void actionNode(int player_index, SMMPlayer *players, int board_nr, int die_result)
{
    SMMPlayer p;
    int current_pos;
    SMMNode current_node;
    int type;
    char* node_name;
    int target;
    int node_energy;
    int current_energy;
    int card_index;
    SMMFoodCard card;
    int card_energy;
    SMMFestCard fest_card;
    char choice;
    int node_credit;
    
    p = players[player_index];
    current_pos = smm_get_player_position(p);
    current_node = (SMMNode)smmdb_getData(LISTNO_NODE, current_pos);
    
    type = smm_get_node_type(current_node);
    node_name = smm_get_node_name(current_node);
    
    // ---------------------------------------------------------------------------------
    // 1. 실험 중인 플레이어 처리
    // ---------------------------------------------------------------------------------
    if (smm_get_player_is_experimenting(p))
    {
        target = smm_get_player_experiment_target_die(p);
        
        printf("  [EXPERIMENT] %s가 실험을 시도합니다. 목표 주사위: %i 이상\n", smm_get_player_name(p), target);

        if (die_result >= target) 
        {
            printf("  [SUCCESS] 주사위 %i! 실험에 성공하여 실험 상태에서 해제됩니다!\n", die_result);
            smm_set_player_is_experimenting(p, 0);
            smm_set_player_experiment_target_die(p, 0);
        }
        else
        {
            printf("  [FAIL] 주사위 %i! 실험에 실패했습니다. 다음 턴에 재시도합니다.\n", die_result);
        }
        
        return; 
    }
    
    // ---------------------------------------------------------------------------------
    // 2. 일반 노드 동작 처리
    // ---------------------------------------------------------------------------------
    
    node_energy = smm_get_node_energy(current_node);
    current_energy = smm_get_player_energy(p);

    switch(type)
    {
        case 0: // 강의 (SMM_LECTURE) - 수강/드랍 선택 추가
        {
            node_credit = smm_get_node_credit(current_node);
            
            // 1. 에너지 체크
            if (current_energy < node_energy)
            {
                printf("  [LECTURE] 에너지가 부족하여 %s 강의를 들을 수 없습니다. (필요: %i, 현재: %i)\n",
                       node_name, node_energy, current_energy);
                break;
            }
            
            // 2. 중복 수강 체크
            if (hasAlreadyTaken(player_index, node_name, players))
            {
                printf("  [LECTURE] %s 강의는 이미 수강했습니다. 드랍합니다.\n", node_name);
                break;
            }
            
            // 3. 수강/드랍 선택
            printf("  [LECTURE] %s 강의를 발견했습니다. (학점: %i, 소요 에너지: %i)\n", 
                   node_name, node_credit, node_energy);
            printf("  수강하시겠습니까? (y/n): ");
            
            fflush(stdin);
            choice = getchar();
            if (choice != '\n' && choice != EOF) {
                while(getchar() != '\n' && getchar() != EOF);
            }
            
            if (choice == 'y' || choice == 'Y')
            {
                // 에너지 차감
                current_energy -= node_energy;
                smm_set_player_energy(p, current_energy);
                printf("  [Energy Loss] 에너지를 %i 소모했습니다. (현재 에너지: %i)\n", 
                       node_energy, current_energy);
                
                // 강의 수강
                takeLecture(player_index, node_name, node_credit, players);
            }
            else
            {
                printf("  [DROP] %s 강의를 드랍했습니다.\n", node_name);
            }
            
            break;
        }
        case 1: // 식당 (SMM_RESTAURANT)
        {
            card_index = rand() % food_nr;
            card = (SMMFoodCard)smmdb_getData(LISTNO_FOODCARD, card_index);
            
            card_energy = smm_get_foodcard_energy(card);
            current_energy = smm_get_player_energy(p) + card_energy;
            smm_set_player_energy(p, current_energy);
            
            printf("  [RESTAURANT] %s에서 %s(카드)를 뽑았습니다. 에너지 %i를 획득/차감합니다. (현재 에너지: %i)\n",
                   node_name, smm_get_foodcard_name(card), card_energy, current_energy);

            break;
        }
        case 2: // 실험실 (SMM_LABORATORY) - 단순 방문 노드
        {
            current_energy -= node_energy;
            smm_set_player_energy(p, current_energy);
            printf("  [LABORATORY] %s에 방문했습니다. 에너지를 %i 소모했습니다. (현재 에너지: %i)\n", 
                   node_name, node_energy, current_energy);
            break;
        }
        case 3: // 집 (SMM_HOME)
        {
            printf("  [HOME] 집에 도착했습니다. 다음 턴을 준비합니다.\n");
            if (smm_get_player_credit(p) >= GRADUATE_CREDIT)
            {
                printf("  [GRADUATE CHECK] 졸업 학점(%i) 충족! 졸업이 가능합니다.\n", smm_get_player_credit(p));
            }

            break;
        }
        case 4: // 실험 (SMM_EXPERIMENT) - 실험 시작
        {
            current_energy -= node_energy;
            smm_set_player_energy(p, current_energy);
            printf("  [EXPERIMENT] 실험 노드에 도착했습니다. 에너지를 %i 소모했습니다. (현재 에너지: %i)\n", 
                   node_energy, current_energy);
            
            // 실험 시작
            target = rand() % MAX_DIE + 1;
            smm_set_player_is_experimenting(p, 1);
            smm_set_player_experiment_target_die(p, target);
            printf("  [EXPERIMENT START] 실험을 시작합니다! 목표 주사위: %i 이상 (다음 턴부터 실험 시도)\n", target);
            
            break;
        }
        case 5: // 보충찬스 (SMM_FOOD_CHANCE)
        {
            printf("  [FOOD CHANCE] 보충 찬스! 음식 카드 효과가 발동됩니다.\n");
            card_index = rand() % food_nr;
            card = (SMMFoodCard)smmdb_getData(LISTNO_FOODCARD, card_index);
            
            card_energy = smm_get_foodcard_energy(card);
            current_energy = smm_get_player_energy(p) + card_energy; 
            smm_set_player_energy(p, current_energy);
            
            printf("  [FOOD CHANCE] %s(카드)를 뽑았습니다. 에너지 %i를 획득/차감합니다. (현재 에너지: %i)\n",
                   smm_get_foodcard_name(card), card_energy, current_energy);
            
            break;
        }
        case 6: // 축제 (SMM_FESTIVAL)
        {
            card_index = rand() % festival_nr;
            fest_card = (SMMFestCard)smmdb_getData(LISTNO_FESTCARD, card_index);
            
            printf("  [FESTIVAL] 축제에 참가했습니다. 미션: \"%s\"\n", smm_get_festcard_content(fest_card));

            break;
        }
        default:
            printf("  [ERROR] 알 수 없는 노드 유형: %i\n", type);
            break;
    }
    
    // 에너지 고갈 체크
    if (smm_get_player_energy(p) <= 0)
    {
       printf("\n  [WARNING] %s의 에너지가 바닥났습니다! (현재: %i)\n", 
           smm_get_player_name(p), smm_get_player_energy(p));
       printf("  에너지가 부족하면 강의 수강이 불가능합니다. 집(HOME)을 통과하여 충전하세요.\n");
    }
}


int rolldie(int player_index, SMMPlayer *players)
{
    char c;
    
    printf(" Press any key to roll a die (press g to see grade): ");
    
    fflush(stdin); 
    c = getchar();
    
    if (c != '\n' && c != EOF) {
        while(getchar() != '\n' && getchar() != EOF);
    }
    
    if (c == 'g' || c == 'G')
    {
        printGrades(player_index, players);
        
        printf(" Press any key to roll a die: ");
        c = getchar();
        if (c != '\n' && c != EOF) {
            while(getchar() != '\n' && getchar() != EOF);
        }
    }
    
    return (rand()%MAX_DIE + 1);
}


float calcAverageGrade(int player_index, SMMPlayer *players)
{
    SMMPlayer p;
    int list_nr;
    int total_lectures;
    float grade_score[9] = {4.3f, 4.0f, 3.7f, 3.3f, 3.0f, 2.7f, 2.3f, 2.0f, 1.7f};
    float total_gpa;
    int i; 
    SMMLectureHistory history;
    int grade_index;
    
    p = players[player_index];
    list_nr = smm_get_player_lecture_list_nr(p);
    total_lectures = smmdb_listCount(list_nr);
    
    if (total_lectures == 0) return 0.0f;

    total_gpa = 0.0f;
    
    for (i = 0; i < total_lectures; i++)
    {
        history = (SMMLectureHistory)smmdb_getData(list_nr, i);
        grade_index = (int)smm_get_lecture_grade(history); 
        
        if (grade_index >= 0 && grade_index < 9)
        {
            total_gpa += grade_score[grade_index];
        }
    }
    
    return total_gpa / total_lectures;
}

SMMGrade_e takeLecture(int player_index, char *lectureName, int credit, SMMPlayer *players)
{
    SMMPlayer p;
    int grade_index;
    SMMGrade_e grade;
    SMMLectureHistory lecture_history;
    int list_nr;
    int new_credit;
    
    p = players[player_index];
    
    grade_index = rand() % 9; 
    grade = (SMMGrade_e)grade_index;
    
    lecture_history = smm_add_lecture_history(lectureName, grade);
    
    list_nr = smm_get_player_lecture_list_nr(p);
    smmdb_addTail(list_nr, lecture_history);
    
    new_credit = smm_get_player_credit(p) + credit;
    smm_set_player_credit(p, new_credit);

    printf("  [GRADE] %s: %s (Credit: %i -> %i)\n", 
           lectureName, smm_get_grade_name(grade), smm_get_player_credit(p) - credit, new_credit);
            
    return grade;
}

void printFinalResult(SMMPlayer *players, int player_nr)
{
    int graduated_player_index;
    int i, j;
    float final_gpa;
    SMMPlayer winner;
    int list_nr;
    int total_lectures;
    SMMLectureHistory history;
    
    printf("\n\n=======================================================\n");
    printf("              ★★ 숙명 모두의마블 최종 결과 ★★                \n");
    printf("=======================================================\n");

    graduated_player_index = -1;
    
    for (i = 0; i < player_nr; i++)
    {
        if (smm_get_player_position(players[i]) == 0 && 
            smm_get_player_credit(players[i]) >= GRADUATE_CREDIT)
        {
            graduated_player_index = i;
            break;
        }
    }

    if (graduated_player_index != -1)
    {
        winner = players[graduated_player_index];
        printf(" ★ 승자: %s (최초 졸업자)\n", smm_get_player_name(winner));
        
        // 졸업자의 수강 이력 출력
        printf("\n--- %s의 수강 이력 ---\n", smm_get_player_name(winner));
        list_nr = smm_get_player_lecture_list_nr(winner);
        total_lectures = smmdb_listCount(list_nr);
        
        printf("강의명 | 학점 | 성적\n");
        printf("------------------------\n");
        
        for (j = 0; j < total_lectures; j++)
        {
            SMMNode lecture_node;
            int k;
            int found_credit = 0;
            
            history = (SMMLectureHistory)smmdb_getData(list_nr, j);
            
            // 강의명으로 학점 찾기 (보드 노드에서 검색)
            for (k = 0; k < board_nr; k++)
            {
                lecture_node = (SMMNode)smmdb_getData(LISTNO_NODE, k);
                if (smm_get_node_type(lecture_node) == 0) // 강의 노드만
                {
                    if (strcmp(smm_get_node_name(lecture_node), smm_get_lecture_name(history)) == 0)
                    {
                        found_credit = smm_get_node_credit(lecture_node);
                        break;
                    }
                }
            }
            
            printf("%s | %i | %s\n", 
                   smm_get_lecture_name(history),
                   found_credit,
                   smm_get_grade_name(smm_get_lecture_grade(history)));
        }
        printf("------------------------\n");
    }
    else
    {
        printf(" 게임이 종료되었지만, 명확한 졸업자가 확인되지 않았습니다.\n");
    }

    printf("\n--- 최종 플레이어 상태 ---\n");
    
    for (i = 0; i < player_nr; i++)
    {
        final_gpa = calcAverageGrade(i, players);
        
        printf("[P%i] %s | 최종 학점: %i | 최종 GPA: %.2f\n",
               i + 1,
               smm_get_player_name(players[i]),
               smm_get_player_credit(players[i]),
               final_gpa);
    }
    printf("---------------------------\n");
}

void printGrades(int player_index, SMMPlayer *players)
{
    SMMPlayer p;
    int list_nr;
    int total_lectures;
    int i;
    SMMLectureHistory history;
    char* lecture_name;
    SMMGrade_e grade;
    
    p = players[player_index];
    list_nr = smm_get_player_lecture_list_nr(p);
    total_lectures = smmdb_listCount(list_nr);

    printf("\n---------- %s's Grade History (Total %i Lectures) ----------\n", 
           smm_get_player_name(p), total_lectures);
    
    if (total_lectures == 0)
    {
        printf("  아직 수강한 강의가 없습니다.\n");
    }
    else
    {
        for (i = 0; i < total_lectures; i++)
        {
            history = (SMMLectureHistory)smmdb_getData(list_nr, i);
            lecture_name = smm_get_lecture_name(history);
            grade = smm_get_lecture_grade(history);
            
            printf("  [%i] %s: %s\n", i + 1, lecture_name, smm_get_grade_name(grade));
        }
    }
    printf("----------------------------------------------------------\n");
}


int main(int argc, const char * argv[]) {
    
    FILE* fp = NULL;
    // 파일 입출력 시 사용할 변수 정의
    char name[MAX_CHARNAME];
    int type;
    int credit;
    int energy;
    char content[MAX_CHARNAME]; // 축제 카드용
    size_t len; // 문자열 길이 계산용
    
    // 플레이어 초기 에너지 저장을 위한 변수
    int initial_player_energy; // 초기화는 아래에서 수행
    
    // 2. Player configuration
    int player_nr;
    char player_name[MAX_CHARNAME];
    SMMPlayer players[MAX_PLAYER]; // 플레이어 객체를 저장할 배열
    int i; // 루프 변수
    
    // 3. SM Marble game starts
    int current_player_index; // 0번 플레이어부터 시작
    int die_result;
    SMMPlayer current_player;
    char* current_player_name;
    
    
    // 변수 초기화 블록
    initial_player_energy = 0;
    board_nr = 0;
    food_nr = 0;
    festival_nr = 0;
    player_nr = 0;
    current_player_index = 0;
    
    srand((unsigned)time(NULL)); // time.h를 사용하므로 명시적인 (unsigned) 캐스팅 추가
    
    
    //1. import parameters ---------------------------------------------------------------------------------
    //1-1. boardConfig 
    if ((fp = fopen(BOARDFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", BOARDFILEPATH);
        getchar();
        return -1;
    }
    
    printf("Reading board component......\n");
    // (노드 이름) (노드 유형) (학점) (소요/보충 에너지)
    while (fscanf(fp, "%s %i %i %i", name, &type, &credit, &energy) == 4) //read a node parameter set
    {
        SMMNode newNode = smm_create_node(name, type, credit, energy);
        smmdb_addTail(LISTNO_NODE, newNode);
        
        // 집 노드(유형 3)의 보충 에너지를 플레이어 초기 에너지로 저장
        if (board_nr == 0 && type == 3) // 첫 번째 노드가 '집'일 때
        {
            initial_player_energy = energy;
        }

        board_nr++;
    }
    fclose(fp);
    printf("Total number of board nodes : %i\n", board_nr);
    
    
    
    //1-2. food card config 
    if ((fp = fopen(FOODFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", FOODFILEPATH);
        return -1;
    }
    
    printf("\n\nReading food card component......\n");
    // (음식 이름) (보충 에너지)
    while (fscanf(fp, "%s %i", name, &energy) == 2) //read a food parameter set
    {
        SMMFoodCard newCard = smm_create_foodcard(name, energy);
        smmdb_addTail(LISTNO_FOODCARD, newCard);
        food_nr++;
    }
    fclose(fp);
    printf("Total number of food cards : %i\n", food_nr);
    
    
    
    //1-3. festival card config 
    if ((fp = fopen(FESTFILEPATH,"r")) == NULL)
    {
        printf("[ERROR] failed to open %s. This file should be in the same directory of SMMarble.exe.\n", FESTFILEPATH);
        return -1;
    }
    
    printf("\n\nReading festival card component......\n");
    
    while (fgets(content, MAX_CHARNAME, fp) != NULL) //read a festival card string
    {
        // fgets로 읽은 문자열 끝의 \n 제거
        len = strlen(content); // 선언된 len 변수 사용
        if (len > 0 && content[len-1] == '\n') {
            content[len-1] = '\0';
        }

        // 빈 줄이 아닐 경우에만 저장
        if (strlen(content) > 0)
        {
            SMMFestCard newCard = smm_create_festcard(content);
            smmdb_addTail(LISTNO_FESTCARD, newCard);
            festival_nr++;
        }
    }
    fclose(fp);
    printf("Total number of festival cards : %i\n", festival_nr);
    
    
    
    //2. Player configuration ---------------------------------------------------------------------------------
    
    
    do
    {
        printf("\nInput number of players (1 ~ %i): ", MAX_PLAYER);
        if (scanf("%i", &player_nr) != 1) {
            // 입력 오류 처리
            player_nr = 0;
            while(getchar() != '\n'); // 입력 버퍼 비우기
        }
        while(getchar() != '\n'); // 입력 버퍼 비우기
        
    }
    while (player_nr < 1 || player_nr > MAX_PLAYER);
    
    for (i = 0; i < player_nr; i++)
    {
        printf("Input player %i name: ", i+1);
        if (fgets(player_name, MAX_CHARNAME, stdin) != NULL)
        {
            // fgets로 읽은 문자열 끝의 \n 제거
            len = strlen(player_name); // 선언된 len 변수 사용
            if (len > 0 && player_name[len-1] == '\n') {
                player_name[len-1] = '\0';
            }
        } else {
             // 입력 실패 시 처리
             strncpy(player_name, "Player", MAX_CHARNAME - 1);
             player_name[5] = i + '1';
             player_name[6] = '\0';
        }
        
        // 플레이어 객체 생성 및 배열에 저장
        // LISTNO_OFFSET_GRADE + i : 플레이어별 수강 이력 리스트 번호 할당
        players[i] = smm_create_player(player_name, initial_player_energy, LISTNO_OFFSET_GRADE + i);
    }


    //3. SM Marble game starts ---------------------------------------------------------------------------------
    
    printf("\n\n==================== 숙명 모두의마블 게임 시작 ====================\n");
    
    // player_nr은 이미 //2. Player configuration에서 정의되었습니다.
    while (!isGraduated(players, player_nr, board_nr)) // 졸업자가 나올 때까지 반복
    {
        current_player = players[current_player_index];
        current_player_name = smm_get_player_name(current_player);
        
        printf("\n\n======== %s's Turn (P%i) (Credit: %i) ========\n", 
               current_player_name, 
               current_player_index + 1, 
               smm_get_player_credit(current_player));
        
        // 4-1. initial printing (턴 시작 시 전체 상태 출력)
        printPlayerStatus(players, player_nr, board_nr);
        
        // 4-2. die rolling (if not in experiment)
        if (smm_get_player_is_experimenting(current_player))
        {
            printf(">> %s (P%i)는 실험 중입니다. 이동 불가. 실험을 시도합니다.\n", current_player_name, current_player_index + 1);
            // rolldie 함수 인자 수정 (player_index, players)
            die_result = rolldie(current_player_index, players); 
            printf(">> 주사위 결과: %i\n", die_result);
            
            // 실험 중: 이동 없이 현재 위치에서 actionNode 실행
            actionNode(current_player_index, players, board_nr, die_result); 
        }
        else
        {
            // 일반 상태: 주사위를 굴리고 이동합니다.
            // rolldie 함수 인자 수정 (player_index, players)
            die_result = rolldie(current_player_index, players); 
            printf(">> 주사위 결과: %i\n", die_result);

            // 4-3. go forward (이동)
            goForward(current_player_index, die_result, players, board_nr); 

            // 4-4. take action at the destination node of the board (노드 액션)
            actionNode(current_player_index, players, board_nr, die_result); 
        }

        // 4-5. next turn (다음 플레이어로 이동)
        current_player_index = (current_player_index + 1) % player_nr;
    }
    
    // 게임 종료 후 결과 출력 로직 
    printFinalResult(players, player_nr);

    // 창이 바로 닫히지 않도록
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf(" 게임이 종료되었습니다. 성적표를 확인한 후 \n");
    printf(" 엔터(Enter) 키를 누르면 창이 닫힙니다.\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    // 입력 버퍼를 비우고 입력을 대기합니다.
    while(getchar() != '\n'); // 혹시 남아있을지 모를 엔터 제거
    getchar();                // 사용자의 입력을 기다림
    
    return 0;
}
    
