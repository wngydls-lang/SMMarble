//
//  main.c
//  SMMarble
//
//

#include <time.h>
#include <string.h>
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



//function prototypes
#if 0
int isGraduated(void); //check if any player is graduated
void generatePlayers(int n, int initEnergy); //generate a new player
void printGrades(int player); //print grade history of the player
void goForward(int player, int step); //make player go "step" steps on the board (check if player is graduated)
void printPlayerStatus(void); //print all player status at the beginning of each turn
float calcAverageGrade(int player); //calculate average grade of the player
smmGrade_e takeLecture(int player, char *lectureName, int credit); //take the lecture (insert a grade of the player)
void* findGrade(int player, char *lectureName); //find the grade from the player's grade history
void printGrades(int player); //print all the grade history of the player
#endif

// =================================================================================================
// PHASE 3 구현 함수 시작
// =================================================================================================

// check if any player is graduated
// 졸업 조건: GRADUATE_CREDIT 이상 이수 + 집 노드(유형 3)에 위치
int isGraduated(SMMPlayer *players, int player_nr, int board_nr)
{
    // 0번 노드는 집 노드이므로, DB에서 가져올 필요 없이 0번 위치 확인만으로 충분합니다.
    
    for (int i = 0; i < player_nr; i++)
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
    printf("\n\n-------------------------------------------------------\n");
    printf("                  CURRENT PLAYER STATUS                \n");
    printf("-------------------------------------------------------\n");
    
    for (int i = 0; i < player_nr; i++)
    {
        SMMPlayer p = players[i];
        int pos = smm_get_player_position(p);
        // DB에서 현재 위치의 노드 정보를 가져옵니다.
        SMMNode current_node = (SMMNode)smmdb_getData(LISTNO_NODE, pos);
        
        // GPA 계산 (Phase 5 반영)
        float gpa = calcAverageGrade(i, players);
        
        printf("[P%i] %s (E: %i, C: %i, GPA: %.2f) | Location: %i(%s)",
               i + 1,
               smm_get_player_name(p),
               smm_get_player_energy(p),
               smm_get_player_credit(p),
               gpa, // GPA 출력
               pos,
               smm_get_node_name(current_node));

        if (smm_get_player_is_experimenting(p))
        {
            printf(" [?? EXPERIMENTING: Target %i]", smm_get_player_experiment_target_die(p));
        }
        
        printf("\n");
    }
    printf("-------------------------------------------------------\n");
}

// =================================================================================================
// PHASE 3 구현 함수 끝
// =================================================================================================

// make player go "step" steps on the board (check if player is graduated)
void goForward(int player_index, int step, SMMPlayer *players, int board_nr)
{
    SMMPlayer p = players[player_index];
    int current_pos = smm_get_player_position(p);
    int current_energy = smm_get_player_energy(p);
    
    // 집 노드(0번)의 보충 에너지를 가져옵니다. (첫 번째 노드는 항상 집 노드)
    SMMNode home_node = (SMMNode)smmdb_getData(LISTNO_NODE, 0);
    int home_energy = smm_get_node_energy(home_node);

    printf(">> %s (P%i)가 %i칸 이동합니다. 현재 위치: %i\n", 
           smm_get_player_name(p), player_index + 1, step, current_pos);

    for (int i = 1; i <= step; i++)
    {
        int next_pos = (current_pos + 1) % board_nr;
        SMMNode next_node = (SMMNode)smmdb_getData(LISTNO_NODE, next_pos);

        // 집 노드를 통과할 때 (0번 노드를 밟을 때)
        if (next_pos == 0)
        {
            current_energy += home_energy;
            smm_set_player_energy(p, current_energy);
            printf("  [PASS HOME] %s(%i) 노드 통과! 에너지를 %i 보충 받습니다. (현재 에너지: %i)\n", 
                   smm_get_node_name(next_node), next_pos, home_energy, current_energy);
        }

        current_pos = next_pos;
    }
    
    // 최종 위치 업데이트
    smm_set_player_position(p, current_pos);
    SMMNode final_node = (SMMNode)smmdb_getData(LISTNO_NODE, current_pos);
    printf(">> 최종 도착 위치: %i(%s) \n", current_pos, smm_get_node_name(final_node));
}

// action code when a player stays at a node
void actionNode(int player_index, SMMPlayer *players, int board_nr, int die_result)
{
    SMMPlayer p = players[player_index];
    int current_pos = smm_get_player_position(p);
    SMMNode current_node = (SMMNode)smmdb_getData(LISTNO_NODE, current_pos);
    
    int type = smm_get_node_type(current_node);
    char* node_name = smm_get_node_name(current_node);
    
    // ---------------------------------------------------------------------------------
    // 1. 실험 중인 플레이어 처리 (실험실 노드를 밟지 않았어도 모든 턴에 실행)
    // ---------------------------------------------------------------------------------
    if (smm_get_player_is_experimenting(p))
    {
        int target = smm_get_player_experiment_target_die(p);
        
        printf("  [?? EXPERIMENT] %s가 실험을 시도합니다. 목표 주사위: %i 이상\n", smm_get_player_name(p), target);

        // 실험 성공: 주사위 결과 >= 목표값
        if (die_result >= target) 
        {
            printf("  [SUCCESS] 주사위 %i! 실험에 성공하여 실험 상태에서 해제됩니다!\n", die_result);
            smm_set_player_is_experimenting(p, 0); // 실험 상태 해제
            smm_set_player_experiment_target_die(p, 0);
            
            // 실험실 노드를 비활성화 상태로 만듭니다. (실험 성공 시 해당 실험실 노드 재활성화 필요)
            // 실험실은 2번 노드(전자공학실험실) 하나뿐이므로, 2번 노드에만 적용합니다.
            SMMNode lab_node = (SMMNode)smmdb_getData(LISTNO_NODE, 8); // '전자공학실험실'은 8번 노드
            smm_set_node_is_laboratory_active(lab_node, 0); // 실험 성공 시 실험실 비활성화
            
        }
        else // 실험 실패: 주사위 결과 < 목표값
        {
            printf("  [FAIL] 주사위 %i! 실험에 실패했습니다. 다음 턴에 재시도합니다.\n", die_result);
        }
        
        // 실험 중인 플레이어는 노드 액션 수행 없이 턴 종료
        return; 
    }
    
    // ---------------------------------------------------------------------------------
    // 2. 일반 노드 동작 처리 (실험 중이 아닐 때만 실행)
    // ---------------------------------------------------------------------------------
    
    // 노드 도착 시 소요 에너지 차감 (집/식당/보충찬스/축제 제외)
    int node_energy = smm_get_node_energy(current_node);
    int current_energy = smm_get_player_energy(p);
    
    if (type == 0 || type == 2 || type == 4) // 강의(0), 실험실(2), 실험(4)
    {
        current_energy -= node_energy;
        smm_set_player_energy(p, current_energy);
        printf("  [Energy Loss] %s(%s)에서 에너지를 %i 소모했습니다. (현재 에너지: %i)\n", 
               node_name, smm_get_node_name(current_node), node_energy, current_energy);
    }
    
    // 에너지 부족 시 게임 종료
    if (current_energy <= 0)
    {
        printf("\n\n=======================================================\n");
        printf("  [ENERGY OUT] %s의 에너지가 모두 소진되었습니다. 게임 오버. ", smm_get_player_name(p));
        printf("\n=======================================================\n");
        smm_set_player_energy(p, 0); // 에너지를 0으로 설정
        // 이 플레이어는 턴을 넘기며 다음 턴에 다시 에너지가 0이면 게임 종료 처리가 필요하지만,
        // 단순화하여 노드 도착 시 0 이하가 되면 강제 종료하는 것으로 처리합니다. (추후 졸업/에너지 아웃 분기 필요)
    }


    switch(type)
    {
        case 0: // 강의 (SMM_LECTURE)
        {
            // 강의 수강 및 성적 부여 함수 호출
            takeLecture(player_index, node_name, smm_get_node_credit(current_node), players);
            break;
        }
        case 1: // 식당 (SMM_RESTAURANT)
        {
            // 식당 노드 처리 (음식 카드 뽑기)
            int card_index = rand() % food_nr;
            SMMFoodCard card = (SMMFoodCard)smmdb_getData(LISTNO_FOODCARD, card_index);
            
            int card_energy = smm_get_foodcard_energy(card);
            current_energy = smm_get_player_energy(p) + card_energy; // 소모 전에 이미 에너지 소모가 없으므로 현재 에너지에 더함
            smm_set_player_energy(p, current_energy);
            
            printf("  [RESTAURANT] %s에서 %s(카드)를 뽑았습니다. 에너지 %i를 획득/차감합니다. (현재 에너지: %i)\n",
                   node_name, smm_get_foodcard_name(card), card_energy, current_energy);

            break;
        }
        case 2: // 실험실 (SMM_LABORATORY)
        {
            // 실험실 노드 처리 (실험 시작)
            if (smm_get_node_is_laboratory_active(current_node) == 0) // 비활성화 상태 (실험 불가)
            {
                printf("  [LAB] 실험실은 비활성화 상태입니다. 다음 턴에 다시 시도해야 합니다.\n");
            }
            else
            {
                // 실험 시작 로직:
                int target_die = rand() % MAX_DIE + 1; // 1~6 중 하나
                smm_set_player_is_experimenting(p, 1);
                smm_set_player_experiment_target_die(p, target_die);
                printf("  [LAB] 실험을 시작합니다! 목표 주사위: %i 이상 (다음 턴부터 실험 시도)\n", target_die);
                
                // 실험실 노드 비활성화
                smm_set_node_is_laboratory_active(current_node, 0); 
            }
            break;
        }
        case 3: // 집 (SMM_HOME) - 이동 로직에서 이미 보충
        {
            printf("  [HOME] 집에 도착했습니다. 다음 턴을 준비합니다.\n");
            // 졸업 조건을 다시 한번 체크합니다.
            if (smm_get_player_credit(p) >= GRADUATE_CREDIT)
            {
                printf("  [GRADUATE CHECK] 졸업 학점(%i) 충족! 졸업이 가능합니다.\n", smm_get_player_credit(p));
            }

            break;
        }
        case 4: // 실험 (SMM_EXPERIMENT) - 실험실로 이동
        {
            printf("  [EXPERIMENT] 실험에 걸려 실험실(%s)로 이동합니다.\n", smm_get_node_name(smmdb_getData(LISTNO_NODE, 8)));
            smm_set_player_position(p, 8); // '전자공학실험실'은 8번 노드
            
            // 실험실로 이동했으므로, 다시 실험실 노드 액션(case 2)을 수행합니다.
            // 하지만 이중 액션을 방지하기 위해 여기서는 이동만 처리합니다.
            // (주의: 문제 정의서에 '실험 노드 도착 시 실험실로 이동'만 명시되어 있으므로, 이동 후 별도 액션은 취하지 않습니다.)
            break;
        }
        case 5: // 보충찬스 (SMM_FOOD_CHANCE) - 음식 카드와 동일
        {
            printf("  [FOOD CHANCE] 보충 찬스! 음식 카드 효과가 발동됩니다.\n");
            // 식당 노드와 동일한 로직을 수행합니다. (코드 복사)
            int card_index = rand() % food_nr;
            SMMFoodCard card = (SMMFoodCard)smmdb_getData(LISTNO_FOODCARD, card_index);
            
            int card_energy = smm_get_foodcard_energy(card);
            current_energy = smm_get_player_energy(p) + card_energy; 
            smm_set_player_energy(p, current_energy);
            
            printf("  [FOOD CHANCE] %s(카드)를 뽑았습니다. 에너지 %i를 획득/차감합니다. (현재 에너지: %i)\n",
                   smm_get_foodcard_name(card), card_energy, current_energy);
            
            break;
        }
        case 6: // 축제 (SMM_FESTIVAL)
        {
            // 축제 노드 처리 (축제 카드 뽑기)
            int card_index = rand() % festival_nr;
            SMMFestCard card = (SMMFestCard)smmdb_getData(LISTNO_FESTCARD, card_index);
            
            printf("  [FESTIVAL] 축제에 참가했습니다. 미션: \"%s\"\n", smm_get_festcard_content(card));
            
            // 축제 노드의 동작 (에너지 소모/보충 없음)

            break;
        }
        default:
            printf("  [ERROR] 알 수 없는 노드 유형: %i\n", type);
            break;
    }
}


int rolldie(int player_index, SMMPlayer *players) // 인자 변경: player -> player_index, players 추가
{
    char c;
    printf(" Press any key to roll a die (press g to see grade): ");
    
    // 이전에 getchar()로 받은 문자가 버퍼에 남아있을 수 있으므로 먼저 비워줍니다.
    fflush(stdin); 
    
    // 사용자 입력 받기
    c = getchar();
    
    // 입력 버퍼에 개행 문자가 남아있을 수 있으므로 버퍼를 비워줍니다.
    if (c != '\n' && c != EOF) {
        while(getchar() != '\n' && getchar() != EOF);
    }
    
    if (c == 'g' || c == 'G') // 'g' 또는 'G' 입력 시
    {
        printGrades(player_index, players); // 수정된 printGrades 함수 호출
        
        // 성적 출력 후 다시 주사위를 굴릴 기회를 줍니다.
        printf(" Press any key to roll a die: ");
        
        // 다시 입력 받기
        c = getchar();
        if (c != '\n' && c != EOF) {
            while(getchar() != '\n' && getchar() != EOF);
        }
    }
    
    // 주사위 굴림 결과 반환
    return (rand()%MAX_DIE + 1);
}


// calculate average grade of the player
// 평점 기준: A+ 4.3, A0 4.0, A- 3.7, B+ 3.3, B0 3.0, B- 2.7, C+ 2.3, C0 2.0, C- 1.7
float calcAverageGrade(int player_index, SMMPlayer *players)
{
    SMMPlayer p = players[player_index];
    int list_nr = smm_get_player_lecture_list_nr(p);
    int total_lectures = smmdb_listCount(list_nr);
    
    if (total_lectures == 0) return 0.0f;

    // 평점 테이블: SMM_APLUS(0)부터 SMM_CMINUS(8)까지 매핑
    float grade_score[9] = {4.3f, 4.0f, 3.7f, 3.3f, 3.0f, 2.7f, 2.3f, 2.0f, 1.7f};
    
    float total_gpa = 0.0f;
    
    for (int i = 0; i < total_lectures; i++)
    {
        SMMLectureHistory history = (SMMLectureHistory)smmdb_getData(list_nr, i);
        // SMMGrade_e는 int로 정의되어 있으므로 그대로 사용
        int grade_index = (int)smm_get_lecture_grade(history); 
        
        if (grade_index >= 0 && grade_index < 9)
        {
            total_gpa += grade_score[grade_index];
        }
    }
    
    return total_gpa / total_lectures;
}

// take the lecture (insert a grade of the player)
// 학점에 따라 성적을 랜덤으로 부여하고, 수강 이력을 DB에 저장합니다.
SMMGrade_e takeLecture(int player_index, char *lectureName, int credit, SMMPlayer *players)
{
    SMMPlayer p = players[player_index];
    
    // 1. 성적 랜덤 결정 (A+ ~ C-)
    // 0: A+ ~ 8: C- (총 9가지 성적)
    int grade_index = rand() % 9; 
    SMMGrade_e grade = (SMMGrade_e)grade_index;
    
    // 2. 수강 이력 객체 생성
    SMMLectureHistory lecture_history = smm_add_lecture_history(lectureName, grade);
    
    // 3. 수강 이력 DB에 추가 (플레이어 고유 리스트 번호 사용)
    int list_nr = smm_get_player_lecture_list_nr(p);
    smmdb_addTail(list_nr, lecture_history);
    
    // 4. 플레이어 총 학점 갱신
    int new_credit = smm_get_player_credit(p) + credit;
    smm_set_player_credit(p, new_credit);

    printf("  [GRADE] %s: %s (Credit: %i -> %i)\n", 
           lectureName, smm_get_grade_name(grade), smm_get_player_credit(p) - credit, new_credit);
           
    return grade;
}

// print final result of the game
void printFinalResult(SMMPlayer *players, int player_nr)
{
    printf("\n\n=======================================================\n");
    printf("             ★★ 숙명 모두의마블 최종 결과 ★★             \n");
    printf("=======================================================\n");

    int graduated_player_index = -1;
    
    // 1. 졸업자 확인 및 승자 결정 (가장 먼저 졸업한 사람이 승자)
    for (int i = 0; i < player_nr; i++)
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
        printf(" ★ 승자: %s (최초 졸업자)\n", smm_get_player_name(players[graduated_player_index]));
    }
    else
    {
        // 이론상 while(!isGraduated) 루프 때문에 여기에 도달하지 않아야 하지만, 안전을 위해 메시지 출력
        printf(" 게임이 종료되었지만, 명확한 졸업자가 확인되지 않았습니다.\n");
    }

    printf("\n--- 최종 플레이어 상태 ---\n");
    
    // 2. 모든 플레이어의 최종 상태 출력 (학점 및 GPA)
    for (int i = 0; i < player_nr; i++)
    {
        float final_gpa = calcAverageGrade(i, players);
        
        printf("[P%i] %s | 최종 학점: %i | 최종 GPA: %.2f\n",
               i + 1,
               smm_get_player_name(players[i]),
               smm_get_player_credit(players[i]),
               final_gpa);
    }
    printf("---------------------------\n");
}

// print all the grade history of the player
void printGrades(int player_index, SMMPlayer *players)
{
    SMMPlayer p = players[player_index];
    int list_nr = smm_get_player_lecture_list_nr(p);
    int total_lectures = smmdb_listCount(list_nr);

    printf("\n---------- %s's Grade History (Total %i Lectures) ----------\n", 
           smm_get_player_name(p), total_lectures);

    if (total_lectures == 0)
    {
        printf("  아직 수강한 강의가 없습니다.\n");
    }
    else
    {
        for (int i = 0; i < total_lectures; i++)
        {
            SMMLectureHistory history = (SMMLectureHistory)smmdb_getData(list_nr, i);
            char* lecture_name = smm_get_lecture_name(history);
            SMMGrade_e grade = smm_get_lecture_grade(history);
            
            // i + 1: 리스트 번호 (1번부터 시작)
            printf("  [%i] %s: %s\n", i + 1, lecture_name, smm_get_grade_name(grade));
        }
    }
    printf("----------------------------------------------------------\n");
}

int main(int argc, const char * argv[]) {
    
    FILE* fp;
    // 파일 입출력 시 사용할 변수 정의
    char name[MAX_CHARNAME];
    int type;
    int credit;
    int energy;
    char content[MAX_CHARNAME]; // 축제 카드용

    // 플레이어 초기 에너지 저장을 위한 변수
    int initial_player_energy = 0;
    
    board_nr = 0;
    food_nr = 0;
    festival_nr = 0;
    
    srand(time(NULL));
    
    
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
    // (띄어쓰기 없는 문자열) - 파일 전체를 한 줄씩 읽는 로직 사용
    while (fgets(content, MAX_CHARNAME, fp) != NULL) //read a festival card string
    {
        // fgets로 읽은 문자열 끝의 \n 제거
        size_t len = strlen(content);
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
    
    int player_nr = 0;
    char player_name[MAX_CHARNAME];
    
    // 플레이어 객체를 저장할 DB 리스트를 별도로 마련해야 하므로, SMMPlayer 포인터 배열을 사용합니다.
    SMMPlayer players[MAX_PLAYER]; 
    
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
    
    
    for (int i = 0; i < player_nr; i++)
    {
        printf("Input player %i name: ", i+1);
        if (fgets(player_name, MAX_CHARNAME, stdin) != NULL)
        {
            // fgets로 읽은 문자열 끝의 \n 제거
            size_t len = strlen(player_name);
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
    
    int current_player_index = 0; // 0번 플레이어부터 시작
    
    printf("\n\n==================== 숙명 모두의마블 게임 시작 ====================\n");
    
    // player_nr은 이미 //2. Player configuration에서 정의되었습니다.
    while (!isGraduated(players, player_nr, board_nr)) // 졸업자가 나올 때까지 반복
    {
        int die_result;
        SMMPlayer current_player = players[current_player_index];
        char* player_name = smm_get_player_name(current_player);
        
        printf("\n\n======== %s's Turn (P%i) (Credit: %i) ========\n", 
               player_name, 
               current_player_index + 1, 
               smm_get_player_credit(current_player));
        
        // 4-1. initial printing (턴 시작 시 전체 상태 출력)
        printPlayerStatus(players, player_nr, board_nr);
        
        // 4-2. die rolling (if not in experiment)
        if (smm_get_player_is_experimenting(current_player))
        {
            printf(">> %s (P%i)는 실험 중입니다. 이동 불가. 실험을 시도합니다.\n", player_name, current_player_index + 1);
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
    
    // 게임 종료 후 결과 출력 로직 (Phase 6 반영)
    printFinalResult(players, player_nr);
    
    return 0;
}
