#pragma once

#include <string>
#include <vector>
#include <ostream>

namespace core
{
    class LDNI
    {
    public:
        struct RangeElement
        {
            int start_, end_;
            int triIndexStart_, triIndexEnd_;
            int startAttribute_, endAttribute_;
            RangeElement* pNext_;

            RangeElement()
            : start_(0), end_(0), pNext_(nullptr)
            , triIndexStart_(-1), triIndexEnd_(-1)
            {}
            
            RangeElement(int _start,int _end,RangeElement* _pNext=nullptr)
            : start_(_start), end_(_end), pNext_(_pNext)
            , triIndexStart_(-1), triIndexEnd_(-1)
            {}

            void ClearStartAttributes(){
                triIndexStart_ = -1;
                startAttribute_ = 0;
            }

            void ClearEndAttributes(){
                triIndexEnd_ = -1;
                endAttribute_ = 0;
            }

            void CopyStartAttributes(const RangeElement* rhs);
            void CopyEndAttributes(const RangeElement* rhs);
        };

    public:
        LDNI();
        LDNI(const LDNI& rhs);
        explicit LDNI(int width, int height, int depth);
        ~LDNI();
        LDNI& operator=(const LDNI& rhs);

        void Init(int width, int height, int depth);
        void Clear();
        void SetOrigin(int x, int y, int z);
        void SetOriginD(double x,double y,double z){
            oriXD_=x; oriYD_=y; oriZD_=z;
        }
        void SetOriginZ(double v){
            oriZD_ = v;
        }

        bool IsValid() const;

        LDNI* Clone() const;

        //add content
        void AddElement(int x, int y, const RangeElement* pElement, int offset); //pElement need to be deleted afterwards
        void ReplaceElement(int x, int y, RangeElement* pElement);  //will use pElement inside (don't delete it)
        void ClearElement();
        void GenerateEvent();
        void ClearEvent();

        void Merge(const LDNI& rhs); //boolean add/merge, will change size/origin
        void Subtract(const LDNI& rhs); //boolean subtract, will NOT change size/origin

        std::vector< std::vector<std::pair<int, int> > >& GetStartEvent();
        const std::vector< std::vector<std::pair<int, int> > >& GetStartEvent() const;
        std::vector< std::vector<std::pair<int, int> > >& GetEndEvent();
        const std::vector< std::vector<std::pair<int, int> > >& GetEndEvent() const;
        const std::vector<std::pair<int, int> >& GetStartEvent(int z) const;
        const std::vector<std::pair<int, int> >& GetEndEvent(int z) const;

        int GetWidth() const;
        int GetHeight() const;
        int GetDepth() const;
        void SetDepth(int v){depth_ = v;}
        int GetOriX() const;
        int GetOriY() const;
        int GetOriZ() const;
        double GetOriXD() const{return oriXD_;}
        double GetOriYD() const{return oriYD_;}
        double GetOriZD() const{return oriZD_;}

        const std::vector<RangeElement*>& GetElement() const;
        std::vector<RangeElement*>& GetElement();
        bool IsSet(int x, int y, int z) const;  //x, y, z in this LDNI's local coordinate

        //temperory Serializer for demo purpose
        void Serialize(std::ostream& os);

        //for debug
        void Dump(const std::wstring& folder);
        void Dump2(const std::wstring& folder,int z = -1,const wchar_t* prefix=L"",bool bUseZCnt=false);
    private:
        std::vector<RangeElement*> elements_;
        int width_, height_, depth_;
        int oriX_, oriY_, oriZ_;
        double oriXD_ = 0, oriYD_ = 0, oriZD_ = 0;
        std::vector< std::vector<std::pair<int, int> > > startEvent_, endEvent_;
    };
}
