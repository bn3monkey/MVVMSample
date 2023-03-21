#ifndef BN3MONKEY_LIBRARY_H
#define BN3MONKEY_LIBRARY_H

#ifdef _WIN32 //windows
    #ifdef BN3MONKEYLIB_EXPORTS
        #define BN3MONKEYLIBRARY_API __declspec(dllexport)
    #else
        #define BN3MONKEYLIBRARY_API __declspec(dllimport)
    #endif
#else // Linux
    #define BN3MONKEYLIBRARY_API __attribute__((visibility("default")))
#endif

namespace Bn3monkey
{
    namespace Global
    {
        // @brief  라이브러리를 초기화한다.
        // @param data_path  사용하는 데이터의 경로
        // @return  라이브러리 초기화의 성공 여부
        bool BN3MONKEYLIBRARY_API initialize(const char* data_path);

        // @brief 라이브러리에 할당된 자원을 해제한다.
        void BN3MONKEYLIBRARY_API release();

        // @brief 라이브러리의 초기화 여부를 확인한다.
        // @return 라이브러리의 초기화 여부
        bool BN3MONKEYLIBRARY_API isInitialized();
    }  

    /* 성공 여부를 실행 즉시 알 수 있는 건 boolean으로 처리한다.*/

    /* 메모리에 저장되는 프로퍼티 */
    // 실수, 정수형
    // void setProperty(Type value) Type getProperty()
    // 문자형 (메모리 에러라던가 바로 체크 가능)
    // bool setProperty(const char* value) bool getProperty(char* value)
    // 배열 (index에 벗어나는 것으로 바로 체크 가능)
    // size_t getTypeArrayLength()
    // bool setTypeArrayPropertyElement(size_t index, Type value) bool getTypeArrayPropertyElement(size_t index, Type* value)
    // bool setTypeArrayProperty(Type* value, size_t offset, size_t length) bool getTypeArrayProperty(Type* value, size_t* length)

    /* 파일에 저장되는 프로퍼티*/ 
    // 실수, 정수형 (저장할 파일이 안 열리는 것으로 바로 체크 가능) 
    // bool setProperty(const int id, const Type value);
    // bool getProperty(const int id, Type* value);
    // 문자형 (저장할 파일이 안 열리는 것으로 바로 체크 가능)
    // bool setString(const int id, const char* value);
    // bool getString(const int id, char* value, size_t length);
    // 배열형
    

    namespace Property
    {
        // @brief 실수형의 프로퍼티를 설정한다. 이 작업은 비동기적으로 이루어진다.
        //        작업이 완료되면 reigsterDoubleProprertyChanged에 등록된 콜백이 있으면 호출된다.
        // @param 새로 변경하고 싶은 실수형의 변수
        void BN3MONKEYLIBRARY_API setDoubleProperty(const double value);

        // @brief 동기적으로 실수형의 프로퍼티의 값을 가져온다. 가장 최신의 값을 가져오기 위해, 대기가 필요하면 기다린다.
        // @return 가장 최신의 실수형 프로퍼티의 값
        double BN3MONKEYLIBRARY_API getDoubleProperty();

        // @brief 실수형 프로퍼티 변경이 완료되었을 때, 실행하는 콜백을 등록한다.
        // @param 실수형 프로퍼티 변경이 완료되었을 때 실행하는 콜백
        void BN3MONKEYLIBRARY_API registerDoublePropertyChanged(void (*callback)(double value));

        // @brief 정수형의 프로퍼티를 설정한다. 이 작업은 비동기적으로 이루어진다.
        //        작업이 완료되면 registerIntPropertyChanged 등록된 콜백이 있으면 호출된다.
        // @param 새로 변경하고 싶은 정수형의 변수
        void BN3MONKEYLIBRARY_API setIntProperty(const int value);

        // @brief 동기적으로 정수형의 프로퍼티의 값을 가져온다. 가장 최신의 값을 가져오기 위해, 대기가 필요하면 기다린다.
        // @return 가장 최신의 정수형 프로퍼티의 값
        int BN3MONKEYLIBRARY_API getIntProperty();

        // @brief 정수형 프로퍼티 변경이 완료되었을 때, 실행하는 콜백을 등록한다.
        // @param 정수형 프로퍼티 변경이 완료되었을 때 실행하는 콜백
        void BN3MONKEYLIBRARY_API registerIntPropertyChanged(void (*callback)(int value));

        // @brief 문자형의 프로퍼티를 설정한다. 이 작업은 비동기적으로 이루어진다.
        //        작업이 완료되면 registerStringPropertyChanged 등록된 콜백이 있으면 호출된다.
        // @param 새로 변경하고 싶은 문자형의 변수
        // @return 성공 여부
        bool BN3MONKEYLIBRARY_API setStringProperty(const char* value);

        // @brief 동기적으로 문자형의 프로퍼티의 값을 가져온다. 가장 최신의 값을 가져오기 위해, 대기가 필요하면 기다린다.
        // @param 가장 최신의 문자형 프로퍼티의 값
        // @param 가장 최신의 문자형 프로퍼티의 길이
        // @return 성공 여부
        bool BN3MONKEYLIBRARY_API getStringProperty(char* value, size_t length);

        // @brief 문자형 프로퍼티 변경이 완료되었을 때, 실행하는 콜백을 등록한다.
        // @param 문자형 프로퍼티 변경이 완료되었을 때 실행하는 콜백
        void BN3MONKEYLIBRARY_API registerStringPropertyChanged(void (*callback)(const char* value, size_t length));

        // @brief 정수 배열형 프로퍼티의 원소 길이를 가져온다.
        // @return 정수 배열형 프로퍼티의 원소의 길이
        size_t BN3MONKEYLIBRARY_API getIntArrayPropertyLength();

        // @brief 정수 배열형 프로퍼티의 특정 원소를 변경한다.
        //         작업이 완료되면 registerIntArrayPropertyChanged에 등록된 콜백이 있으면 실행된다.
        // @param index 변경하고 싶은 원소의 색인/위치
        // @param value 변경하고자 하는 값
        // @return 성공 여부
        bool BN3MONKEYLIBRARY_API setIntArrayPropertyElement(const size_t index, const int value);

        // @brief 동기적으로 정수 배열형 프로퍼티의 특정 원소 값을 가져온다. 가장 최신의 값을 가져오기 위해, 대기가 필요하면 기다린다.
        // @param index 변경하고 싶은 원소의 색인/위치
        // @param value 특정 원소 값
        // @return 성공 여부        
        bool BN3MONKEYLIBRARY_API getIntArrayPropertyElement(const size_t index, int* value);

        // @brief 정수 배열형 프로퍼티의 값을 일부 변경한다. offset 부터 offset + length -1까지 values에 있는 값으로 변경한다.
        //         작업이 완료되면 registerIntArrayPropertyChanged에 등록된 콜백이 있으면 실행된다.
        // @param values 변경하고자 하는 정수 배열형 값
        // @param offset 변경하고자 하는 정수 배열의 시작값
        // @param length 변경하고자 하는 길이
        // @return 성공 여부
        bool BN3MONKEYLIBRARY_API setIntArrayProperty(const int* values, const size_t offset, const size_t length);

        // @brief 동기적으로 정수 배열형 프로퍼티의 전체 값을 가져온다. 가장 최신의 값을 가져오기 위해, 대기가 필요하면 기다린다.
        // @param values 가장 최신의 정수 배열형 값
        // @param length 가장 최신의 정수 배열의 길이
        // @return 성공 여부
        bool BN3MONKEYLIBRARY_API getIntArrayProperty(int* values, size_t* length);

        // @brief 정수 배열형 값이 변경되었을 떄 실행되는 콜백을 등록한다. 
        // @param callback 정수 배열형 값이 변경되었을 떄 실행되는 콜백. values는 변경된 정수 배열형 값이다. length는 변경된 정수 배열의 길이다.
        //                  changed_index는 변경된 인덱스이다.
        //                  changed_index가 0이면, 여러 개가 변경되었다는 뜻이다. 
        void BN3MONKEYLIBRARY_API registerIntArrayPropertyChanged(void (*callback)(const int* values, const size_t length, const size_t changed_index));
    }


    namespace MultiProperty
    {

        enum BN3MONKEYLIBRARY_API PresetField {
            ID, // 아이디
            NAME, // 이름
            CUSTOM_ORDER, // 임의로 배정한 정렬 순서
        };
        enum BN3MONKEYLIBRARY_API PresetOrder {
            ASEC, // 오름차순
            DESC, // 내림차순
        };

        namespace SurPreset
        {
            // @brief 상위 프리셋을 추가한다. 작업이 완료되었을 경우, registerSurPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param name 추가될 상위 프리셋의 이름
            // @param order 추가될 상위 프리셋의 임의 순서
            // @return 성공 여부
            bool BN3MONKEYLIBRARY_API addSurPreset(const char* name, const double order);
            // @brief 상위 프리셋을 제거한다. 작업이 완료되었을 경우, registerSurPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param value 제거할 상위 프리셋의 id
            // @return 성공 여부
            bool BN3MONKEYLIBRARY_API removeSurPreset(int value);

            // @brief 상위 프리셋의 이름을 변경한다. 작업이 완료되었을 경우, registerSurPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param id 상위 프리셋의 아이디
            // @param name 변경할 상위 프리셋의 이름
            // @return 성공 여부
            bool BN3MONKEYLIBRARY_API setSurPresetName(const int id, const char* value);

            // @brief 상위 프리셋의 이름을 가져온다.
            // @param id 상위 프리셋의 아이디
            // @param value 상위 프리셋의 이름
            // @param length 상위 프리셋 이름을 받아오기 위한 배열의 길이
            // @return 성공 여부.
            bool BN3MONKEYLIBRARY_API getSurPresetName(const int id, char* value, size_t length);

            // @brief 상위 프리셋의 임의 정렬 순서를 변경한다. 작업이 완료되었을 경우, registerSurPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param id 상위 프리셋의 아이디
            // @param value 상위 프리셋의 임의 정렬 순서
            // @return 성공 여부
            bool BN3MONKEYLIBRARY_API setSurPresetOrder(const int id, const double value);

            // @brief 상위 프리셋의 임의 정렬 순서를 가져온다.
            // @param id 상위 프리셋의 아이디
            // @param value 상위 프리셋의 임의 정렬 순서
            // @return 성공 여부
            bool BN3MONKEYLIBRARY_API getSurPresetOrder(const int id, double* value);

            // @brief 현재 접근하고 있는 상위 프리셋을 변경한다. 작업이 완료되었을 경우, registerSurPresetListChanged에 등록된 콜백이 있으면 수행한다. 
            // @param id 변경할 상위 프리셋의 아이디
            void BN3MONKEYLIBRARY_API setCurrentSurPreset(const int value);
            // @brief 현재 접근하고 있는 상위 프리셋을 가져온다.
            // @return 현재 접근하고 있는 상위 프리셋
            int BN3MONKEYLIBRARY_API getCurrentSurPreset();

            // @brief 어플리케이션을 시작할 때, 불러올 상위 프리셋을 설정한다. 작업이 완료되었을 경우, registerSurPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param value 어플리케이션을 시작핼 때 불러올 상위 프리셋
            // @return 성공 여부.  
            bool BN3MONKEYLIBRARY_API setDefaultSurPreset(const int value);
            // @brief 어플리케이션을 시작할 때, 불러올 상위 프리셋을 가져온다.
            // @param value 어플리케이션을 시작핼 때 불러올 상위 프리셋
            // @return 성공 여부.  
            bool BN3MONKEYLIBRARY_API getDefaultSurPreset(int* value);

            // @brief 상위 프리셋의 목록을 가져온다.
            // @param values 상위 프리셋 목록의 id이다.
            // @param field  정렬 기준이 되는 속성이다.
            // @param Order 정렬 방향이다.
            // @return 상위 프리셋 목록의 길이다. 실패했을 경우, 0을 리턴한다.
            size_t BN3MONKEYLIBRARY_API listSurPreset(int* values, const PresetField field, const PresetOrder order);
            // @brief 상위 프리셋 목록이 변경(상위 프리셋 추가, 상위 프리셋 삭제, 상위 프리셋 속성 변경)되었을 때 수행할 콜백을 등록한다.
            // @param field 콜백이 받을 상위 프리셋 목록의 정렬 기준이 되는 속성
            // @param order 콜백이 받을 상위 프리셋 목록의 정령 방향
            // @param callback 상위 프리셋 목록이 변경되었을 때 수행할 콜백. values는 변경된 상위 프리셋 목록, current_surpreset_id는 현재 상위 프리셋의 아이디, default_surpreset_id는 기본 상위 프리셋 아이디이다.
            void BN3MONKEYLIBRARY_API registerSurPresetListChanged(const PresetField field, const PresetOrder order, void (*callback)(int* values, int current_surpreset_id, int default_surpreset_id));
        }
       
        namespace Preset
        {
            // @brief 상위 프리셋에 프리셋을 추가한다. 작업이 완료되었을 경우, registerPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param surpreset_id 프리셋을 추가할 상위 프리셋
            // @param name 추가될 프리셋의 이름
            // @param order 추가될 프리셋의 임의 순서
            // @return 성공 여부. 
            bool BN3MONKEYLIBRARY_API addPreset(const int surpreset_id, const char* name, const double order);
            
            // @brief 상위 프리셋에 있는 프리셋을 제거한다. 작업이 완료되었을 경우, registerPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param surpreset_id 프리셋을 제거할 상위 프리셋
            // @param value 제거할 상위 프리셋의 id
            // @return 성공 여부.
            bool BN3MONKEYLIBRARY_API removePreset(const int surpreset_id, const int value);

            // @brief 프리셋의 이름을 변경한다. 작업이 완료되었을 경우, registerPresetListChanged에 등록된 콜백이 있으면 수행한다.
            // @param id 상위 프리셋의 아이디
            // @param name 변경할 상위 프리셋의 이름
            // @return 성공 여부
            bool BN3MONKEYLIBRARY_API setPresetName(const int id, const char* name);
            // @brief 프리셋의 이름을 가져온다.
            // @param id 상위 프리셋의 아이디
            // @param value 상위 프리셋의 이름
            // @param length 상위 프리셋 이름을 받아오기 위한 배열의 길이
            // @return 성공 여부.
            bool BN3MONKEYLIBRARY_API getPresetName(const int id, char* value, size_t length);
            bool BN3MONKEYLIBRARY_API setPresetOrder(const int id, const double value);
            bool BN3MONKEYLIBRARY_API getPresetOrder(const int id, double* value);

            bool BN3MONKEYLIBRARY_API getCurrentPreset(const int surpreset_id, int* preset_id);

            bool BN3MONKEYLIBRARY_API setDefaultPreset(const int surpreset_id, const int preset_id);
            bool BN3MONKEYLIBRARY_API getDefaultPreset(const int surpreset_id, int* preset_id);

            size_t BN3MONKEYLIBRARY_API listPreset(const int surpreset_id, int* values, const PresetField field, const PresetOrder order);
            void BN3MONKEYLIBRARY_API registerPresetListChanged(const PresetField field, const PresetOrder order, void (*callback)(const int surpreset_id, int* values, int current_preset_id, int default_preset_id));

            bool loadPreset(const int preset_id);
            bool loadPresetAsync(const int preset_id);
        }

        void BN3MONKEYLIBRARY_API registerLoadingChanged(void (*callback)(bool value));
    }
    
    namespace StateMachine
    {
        void BN3MONKEYLIBRARY_API setState(int value);
        int BN3MONKEYLIBRARY_API getState();
        void BN3MONKEYLIBRARY_API registerStateChanged(void (*callback)(int prev, int next));
    }

    namespace Stream
    {
        bool BN3MONKEYLIBRARY_API startStream();
        void BN3MONKEYLIBRARY_API endStream();
        bool BN3MONKEYLIBRARY_API resizeStream(const int width, const int height);
        void BN3MONKEYLIBRARY_API renderStream(void* buffer, size_t size);
        // void BN3MONKEYLIBRARY_API registerStream(void (*callback)(void* buffer, size_t size, int frame_number, long long int timestamp)); 
    }

    namespace Error
    {
        enum class Bn3monkeyErrorCode
        {
            INVALID_PARAM, // 파라미터로 부적절한 값이 들어갔을 때
            INVALID_ID, // 없는 상위 프리셋이나 프리셋 ID에 접근했을 때. 
            OUT_OF_LENGTH, // 배열 범위에서 벗어났을 때
            FILE_OPEN_FAIL, // 라이브러리를 실행하기 위해 필요한 파일이 없을 떄 
        };

        // @brief 리턴 값이 bool일 때, 최근에 발생한 에러를 받아온다.
        void getLastError(Bn3monkeyErrorCode err_num, const char* err_str);
        void BN3MONKEYLIBRARY_API registerErrorCallback(void (*callback)(Bn3monkeyErrorCode err_num, const char* value));
    }
}
#endif // BN3MONKEY_LIBRARY_H