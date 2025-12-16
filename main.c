//
//  main.c
//  SMMarble
//
//  Created by Juyeop Kim on 2023/11/05.
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




int rolldie(int player)
{
    char c;
    printf(" Press any key to roll a die (press g to see grade): ");
    c = getchar();
    fflush(stdin);
    
#if 0
    if (c == 'g')
        printGrades(player);
#endif
    
    return (rand()%MAX_DIE + 1);
}


//action code when a player stays at a node
void actionNode(int player)
{
    switch(type)
    {
        //case lecture:
        default:
            break;
    }
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
    while () //is anybody graduated?
    {
        int die_result;
        
        //4-1. initial printing
        //printPlayerStatus();
        
        //4-2. die rolling (if not in experiment)
        
        
        //4-3. go forward
        //goForward();

		//4-4. take action at the destination node of the board
        //actionNode();
        
        //4-5. next turn
        
    }
    
    return 0;
}
