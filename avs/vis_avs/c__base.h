#pragma once


// base class declaration, compatibility class
class RString;

class C_RBASE {
    public:
        C_RBASE() { }
        virtual ~C_RBASE() { };
        virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)=0; // returns 1 if fbout has dest
        virtual char *get_desc()=0;
        virtual void load_config(unsigned char *data, int len) { }
        virtual int  save_config(unsigned char *data) { return 0; }

        void load_string(RString &s,unsigned char *data, int &pos, int len);
        void save_string(unsigned char *data, int &pos, RString &text);
};

class C_RBASE2 : public C_RBASE {
    public:
        C_RBASE2() { }
        virtual ~C_RBASE2() { };

    int getRenderVer2() { return 2; }


    virtual int smp_getflags() { return 0; } // return 1 to enable smp support

    // returns # of threads you desire, <= max_threads, or 0 to not do anything
    // default should return max_threads if you are flexible
    virtual int smp_begin(int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { return 0; }  
    virtual void smp_render(int this_thread, int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { }; 
    virtual int smp_finish(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { return 0; }; // return value is that of render() for fbstuff etc
};

class RString
{
  private:
    char *m_str;
    int m_size;

  public:
    RString() { m_str=0; m_size=0; }

    ~RString() {
        if (m_str) {
            GlobalFree(m_str);
        }
    }

    void resize(int size) {
        m_size = size;
        if (m_str) {
            GlobalFree(m_str);
        }
        m_str = 0;
        if (size) {
            m_str = (char*)GlobalAlloc(GPTR,size);
        }
    }

    char *get() { return m_str; }

    int getsize() {
        if (!m_str) {
            return 0;
        }
        return m_size;
    }

    void assign(char *s) {
        resize(strlen(s) + 1);
        strcpy(m_str, s);
    }

    void get_from_dlgitem(HWND hwnd, int dlgItem) {
        int l = SendDlgItemMessage(hwnd, dlgItem, WM_GETTEXTLENGTH, 0, 0);
        if (l < 256) {
            l = 256;
        }
        resize(l + 1 + 256);
        GetDlgItemText(hwnd, dlgItem, m_str, l + 1);
        m_str[l] = 0;
    }
};
