#include "test.h"

/* ブロックコメント */

// 行コメント

int main() {
    ASSERT(3, ({ 1; { 2; } 3; }));
    ASSERT(5, ({ ;;; 5; }));

    ASSERT(3, ({ int x; if(0) x=2; else x=3; x; }));
    ASSERT(3, ({ int x; if(1-1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if(1) x=2; else x=3; x; }));
    ASSERT(2, ({ int x; if(2-1) x=2; else x=3; x; }));
    ASSERT(4, ({ int x; if(0){ x=1; x=2; x=3; }else{ x=4; } x; }));
    ASSERT(3, ({ int x; if(1){ x=1; x=2; x=3; }else{ x=4; } x; }));

    ASSERT(5, ({ int i=0; switch(0){ case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(6, ({ int i=0; switch(1){ case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(7, ({ int i=0; switch(2){ case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(0, ({ int i=0; switch(3){ case 0:i=5;break; case 1:i=6;break; case 2:i=7;break; } i; }));
    ASSERT(5, ({ int i=0; switch(0){ case 0:i=5;break; default:i=7; } i; }));
    ASSERT(7, ({ int i=0; switch(1){ case 0:i=5;break; default:i=7; } i; }));
    ASSERT(2, ({ int i=0; switch(1){ case 0:0; case 1:0; case 2:0;i=2; } i; }));
    ASSERT(0, ({ int i=0; switch(3){ case 0:0; case 1:0; case 2:0;i=2; } i; }));
    ASSERT(5, ({ int i=0; switch(0){ case (0*1):i=5;break; } i; }));

    ASSERT(55, ({ int i=0; int j=0; for(i=0; i<=10; i=i+1) j=i+j; j; }));

    ASSERT(10, ({ int i=0; while(i<10){ i=i+1; } i; }));

    ASSERT(3, ({ int i=0; goto a; a:i++; b:i++; c:i++; i; }));
    ASSERT(2, ({ int i=0; goto e; d:i++; e:i++; f:i++; i; }));
    ASSERT(1, ({ int i=0; goto i; g:i++; h:i++; i:i++; i; }));

    ASSERT(3, ({ int i=0; for(;i<10;i++){ if(i == 3) break; } i; }));
    ASSERT(4, ({ int i=0; while(1){ if(i++ == 3) break; } i; }));
    ASSERT(3, ({ int i=0; for(;i<10;i++){ for(;;) break; if(i==3) break; } i; }));
    ASSERT(4, ({ int i=0; while(1){ while(1) break; if(i++ == 3) break; } i; }));

    ASSERT(10, ({ int i=0; int j=0; for(;i<10;i++){ if(i>5) continue; j++; } i; }));
    ASSERT(6, ({ int i=0; int j=0; for(;i<10;i++){ if(i>5) continue; j++; } j; }));
    ASSERT(10, ({ int i=0; int j=0; for(;!i;){ for(;j!=10;j++) continue; break; } j; }));
    ASSERT(11, ({ int i=0; int j=0; while(i++<10){ if(i>5) continue; j++; } i; }));
    ASSERT(5, ({ int i=0; int j=0; while(i++<10){ if(i>5) continue; j++; } j; }));
    ASSERT(11, ({ int i=0; int j=0; while(!i){ while(j++!=10) continue; break; } j; }));

    ASSERT(55, ({ int j=0; for(int i=0;i<=10;i=i+1) j=j+i; j; }));
    ASSERT(3, ({ int i=3; int j=0; for(int i=0;i<=10;i=i+1) j=j+i; i; }));

    ASSERT(7, ({ int i=0; int j=0; do{ j++; }while(i++<6); j; }));
    ASSERT(4, ({ int i=0; int j=0; int k=0; do{ if (++j>3) break; continue; k++; }while(1); j; }));

    ASSERT(1, ({ typedef int foo; goto foo; foo:; 1; }));

    ASSERT(5, ({ int x; if(0.0) x=3; else x=5; x; }));
    ASSERT(3, ({ int x; if(0.1) x=3; else x=5; x; }));
    ASSERT(5, ({ int x=5; if(0.0) x=3; x; }));
    ASSERT(3, ({ int x=5; if(0.1) x=3; x; }));
    ASSERT(10, ({ double i=10.0; int j=0; for(; i; i--, j++); j; }));
    ASSERT(10, ({ double i=10.0; int j=0; do j++; while(--i); j; }));

    return 0;
}
