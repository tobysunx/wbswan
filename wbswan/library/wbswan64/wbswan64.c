#include <stdio.h>
#include <math/affine_transform.h>
#include <math/matrix_utils.h>
#include <wbswan/wbswan.h>

void tobin(uint32_t x, int SIZE) //要和函数声明一致，所以后面也要填int x,int a[]
{
    int a[32] = {0};
    int n = 0, t, k;
    do
    {
        a[n] = x % 2;
        x = (unsigned int)x >> 1; //要考虑到参数int x有可能为负数所以填x=x/2是不行的。
        //如果x>=0的话填x=x/2没有问题，实际上我估计这里出题者的本意希望填x/2，但是如果当x为负数的时候
        //会出错的，要么填 x=(unsigned int)x/2也是可以的，不过 x=(unsigned int)x/2的运行效率远远低于x=(unsigned int)x>>1。这里牵涉的东西比较多，三言两语说不清
        //如果想了解原因，建议回去看谭浩强的强制类型转换、正数和负数的2进制表示、移位3个知识点
        n++;
    } while (x != 0);
    //do...while()这个功能就是把这个数的二进制的位存入这个数组中
    for (k = 0; k < n / 2; k++)
    {
        t = a[k];
        a[k] = a[n - k - 1]; //实现数组中2个数交换
        a[n - k - 1] = t;
        //for循环是为了交换顺序，比如x=11是的二进制码是1011这4个码一次存在a[3] a[2] a[1] a[0]中，而输出的时候是按a[0] a[1] a[2] a[3]这样输出的如果没有这个交换屏幕上看到的会是1101
    }
    for (k = SIZE; k > n; k--)
    {
        printf("%d-", a[k]);
    }
    for (k = 0; k < n; k++)
        printf("%d-", a[k]);
    int num = 0;
    for (int i = 0; i < 32; i++)
    {
        num += a[i];
    }
    // printf("%d\n", num);
    printf("\n");
}


int swan_whitebox_encrypt(const uint8_t *in, uint8_t *out, swan_whitebox_content *swc)
{
    int i;
    swan_wb_unit L[4];
    swan_wb_unit R[4];
    swan_wb_unit LT[4];
    swan_wb_unit RT[4];
    swan_wb_semi *tempL;
    swan_wb_semi *tempR;

    CombinedAffine *B_ptr = swc->B;
    CombinedAffine *C_ptr = swc->C;
    CombinedAffine *P_ptr = swc->P;
    CombinedAffine *PQ_ptr = swc->PQ;
    swan_wb_semi(*lut_ptr)[4][256] = swc->lut;
    MatGf2 switchmat = make_swithlane(32);


    uint8_t tt[4];
    uint8_t dd[4];
    tempL = (swan_wb_semi *)LT;
    tempR = (swan_wb_semi *)RT;

    // *tempL = ApplyAffineToU32(*(P_ptr->combined_affine), *tempL);
    // *tempR = ApplyAffineToU32(*((P_ptr + 1)->combined_affine), *tempR);

    

    L[0] = LT[0] = swc->SE[0][in[0]];
    L[1] = LT[1] = swc->SE[1][in[1]];
    L[2] = LT[2] = swc->SE[2][in[2]];
    L[3] = LT[3] = swc->SE[3][in[3]];

    R[0] = RT[0] = swc->SE[4][in[4]];
    R[1] = RT[1] = swc->SE[5][in[5]];
    R[2] = RT[2] = swc->SE[6][in[6]];
    R[3] = RT[3] = swc->SE[7][in[7]];

    for (i = 0; i < swc->rounds; i++)
    {

        LT[0] = L[0];
        LT[1] = L[1];
        LT[2] = L[2];
        LT[3] = L[3];

        printf("from:\t%08X\n", *tempL);

        // start theta
        *tempL = ApplyAffineToU32(*(B_ptr->combined_affine), *tempL);

        printf("theta:\t%08X\n", *tempL);

        //start beta
       
        uint32_t LC[4];
    
       
        LC[0] = lut_ptr[i][0][LT[0]];
        LC[1] = lut_ptr[i][1][LT[1]];
        LC[2] = lut_ptr[i][2][LT[2]];
        LC[3] = lut_ptr[i][3][LT[3]];
       
    
        *tempL = LC[0] ^ LC[1] ^ LC[2] ^ LC[3];

        printf("beta:\t%08X\n", *tempL);

        //SWITCHLANE

        *tempL = ApplyMatToU32(switchmat, *tempL);
        
        // P for next
        printf("\t%08X\n", *tempL);
        CombinedAffine * tca = NULL;
        if (i==0) {
            tca = P_ptr+1;
        } else {
            tca = P_ptr+1;
        }
        LT[0] = ApplyAffineToU8((tca)->sub_affine[0], LT[0]);
        LT[1] = ApplyAffineToU8((tca)->sub_affine[1], LT[1]);
        LT[2] = ApplyAffineToU8((tca)->sub_affine[2], LT[2]);
        LT[3] = ApplyAffineToU8((tca)->sub_affine[3], LT[3]);
        printf("\t%08X\n", *tempL);
        
   

        //******************************************************************

        swan_wb_unit tempd[4];

        *((swan_wb_semi *)tempd) = ApplyAffineToU32(*((P_ptr + 2)->combined_affine), ApplyAffineToU32(*((P_ptr)->combined_affine_inv), *((uint32_t *)L)));

        L[0] = R[0] ^ LT[0];
        L[1] = R[1] ^ LT[1];
        L[2] = R[2] ^ LT[2];
        L[3] = R[3] ^ LT[3];

        R[0] = tempd[0];
        R[1] = tempd[1];
        R[2] = tempd[2];
        R[3] = tempd[3];

        swan_wb_semi LP;
        if (swc->weak_or_strong) {
            LT[0] = L[0];
            LT[1] = L[1];
            LT[2] = L[2];
            LT[3] = L[3];
            LP = *tempL;
            swan_wb_unit ULP[4];
            swan_wb_semi *ulp_ptr = (swan_wb_semi *) ULP;
            *ulp_ptr = ApplyAffineToU32(*(PQ_ptr->combined_affine), ApplyMatToU32(((P_ptr+1)->combined_affine_inv->linear_map), *tempL));
            *((swan_wb_semi *)L) = *ulp_ptr;

            MatGf2 mat_x = NULL;
            ReAllocatedMatGf2(32, 1, &mat_x);
            InitVecFromBit(LP, mat_x);
            MatGf2Add(GenMatGf2Mul((tca)->combined_affine->linear_map, (tca)->combined_affine_inv->vector_translation), mat_x, &mat_x);
            LP = get32FromVec(mat_x);

        } else {

            MatGf2 mat_x = NULL;
            ReAllocatedMatGf2(32, 1, &mat_x);
            InitVecFromBit(*((swan_wb_semi *)L), mat_x);
            MatGf2Add(GenMatGf2Mul((tca)->combined_affine->linear_map, (tca)->combined_affine_inv->vector_translation), mat_x, &mat_x);
            *((swan_wb_semi *)L) = get32FromVec(mat_x);
        }



        // MatGf2 mat_lp = NULL;
        // if (swc->weak_or_strong) {
        //     LT[0] = L[0];
        //     LT[1] = L[1];
        //     LT[2] = L[2];
        //     LT[3] = L[3];
        //     LP = *tempL;
        //     swan_wb_unit ULP[4];
        //     swan_wb_semi *ulp_ptr = (swan_wb_semi *) ULP;
        //     *ulp_ptr = ApplyAffineToU32(*(PQ_ptr->combined_affine), ApplyMatToU32(((P_ptr+1)->combined_affine_inv->linear_map), *tempL));
        //     L[0] = ULP[0];
        //     L[1] = ULP[1];
        //     L[2] = ULP[2];
        //     L[3] = ULP[3];

        //     // printf("\n%04llX:%04llX\n", *tempL, LP);

        //     // mat_lp = NULL;
        //     // ReAllocatedMatGf2(32, 1, &mat_lp);
        //     // InitVecFromBit(LP, mat_lp);
        //     // MatGf2Add(GenMatGf2Mul((P_ptr + 1)->combined_affine->linear_map, (P_ptr + 1)->combined_affine_inv->vector_translation), mat_lp, &mat_lp);
        //     // LP = get32FromVec(mat_lp);
        // }

       
        B_ptr++;
        P_ptr++;
        i++;


        // ******* next round 

        if (swc->weak_or_strong) {
            *tempL = LP;
        } else {
            LT[0] = L[0];
            LT[1] = L[1];
            LT[2] = L[2];
            LT[3] = L[3];
        }

        printf("before theta: \n\t%08X\n", *tempL);

        // start theta
        *tempL = ApplyAffineToU32(*(B_ptr->combined_affine), *tempL);

        printf("after theta: \n\t%08X\n", *tempL);
        //start beta
        //back to 8 in 8 out
        //******************************************************************


        //******************************************************************

        //******************************************************************    
       
        LC[0] = lut_ptr[i][0][LT[0]];
        LC[1] = lut_ptr[i][1][LT[1]];
        LC[2] = lut_ptr[i][2][LT[2]];
        LC[3] = lut_ptr[i][3][LT[3]];
       
    
        *tempL = LC[0] ^ LC[1] ^ LC[2] ^ LC[3];

    

        //SWITCHLANE

        *tempL = ApplyMatToU32(switchmat, *tempL);

        // P for next
        LT[0] = ApplyAffineToU8((P_ptr + 1)->sub_affine[0], LT[0]);
        LT[1] = ApplyAffineToU8((P_ptr + 1)->sub_affine[1], LT[1]);
        LT[2] = ApplyAffineToU8((P_ptr + 1)->sub_affine[2], LT[2]);
        LT[3] = ApplyAffineToU8((P_ptr + 1)->sub_affine[3], LT[3]);
   
    




        //******************************************************************

        if (swc->weak_or_strong){
            *((swan_wb_semi *)tempd) = ApplyAffineToU32(*((P_ptr + 2)->combined_affine), ApplyAffineToU32(*((PQ_ptr)->combined_affine_inv), *((uint32_t *)L)));
        } else {
            *((swan_wb_semi *)tempd) = ApplyAffineToU32(*((P_ptr + 2)->combined_affine), ApplyAffineToU32(*((P_ptr)->combined_affine_inv), *((uint32_t *)L)));
        }

        L[0] = R[0] ^ LT[0];
        L[1] = R[1] ^ LT[1];
        L[2] = R[2] ^ LT[2];
        L[3] = R[3] ^ LT[3];

        R[0] = tempd[0];
        R[1] = tempd[1];
        R[2] = tempd[2];
        R[3] = tempd[3];

        mat_x = NULL;
        ReAllocatedMatGf2(32, 1, &mat_x);
        InitVecFromBit(*((uint32_t *)L), mat_x);
        MatGf2Add(GenMatGf2Mul((P_ptr + 1)->combined_affine->linear_map, (P_ptr + 1)->combined_affine_inv->vector_translation), mat_x, &mat_x);
        *((swan_wb_semi *)L) = get32FromVec(mat_x);

        B_ptr++;
        P_ptr++;

        PQ_ptr++;

       

        
    }

    // *((swan_wb_semi *)L) = ApplyAffineToU32(*((P_ptr + 0)->combined_affine_inv), *((swan_wb_semi *)L));
    // *((swan_wb_semi *)R) = ApplyAffineToU32(*((P_ptr + 1)->combined_affine_inv), *((swan_wb_semi *)R));


    out[0] = swc->EE[0][L[0]];
    out[1] = swc->EE[1][L[1]];
    out[2] = swc->EE[2][L[2]];
    out[3] = swc->EE[3][L[3]];

    out[4] = swc->EE[4][R[0]];
    out[5] = swc->EE[5][R[1]];
    out[6] = swc->EE[6][R[2]];
    out[7] = swc->EE[7][R[3]];
    

    // out[0] = L[0];
    // out[1] = L[1];
    // out[2] = L[2];
    // out[3] = L[3];

    // out[4] = R[0];
    // out[5] = R[1];
    // out[6] = R[2];
    // out[7] = R[3];

    return 0;
}

int swan_whitebox_decrypt(const uint8_t *in, uint8_t *out, swan_whitebox_content *swc)
{
    int i;
    swan_wb_unit L[4];
    swan_wb_unit R[4];
    swan_wb_unit LT[4];
    swan_wb_unit RT[4];
    swan_wb_semi *tempL;
    swan_wb_semi *tempR;
 
    uint8_t dd[4];
    uint8_t tt[4];
    CombinedAffine *B_ptr = swc->B;
    CombinedAffine *C_ptr = swc->C;
    CombinedAffine *P_ptr = swc->P;
    CombinedAffine *PQ_ptr = swc->PQ;
    MatGf2 switchmat = make_swithlane(32);
    swan_wb_semi(*lut_ptr)[4][256] = swc->lut;

    LT[0] = in[0];
    LT[1] = in[1];
    LT[2] = in[2];
    LT[3] = in[3];

    RT[0] = in[4];
    RT[1] = in[5];
    RT[2] = in[6];
    RT[3] = in[7];

    tempL = (swan_wb_semi *)LT;
    tempR = (swan_wb_semi *)RT;

    *tempR = ApplyAffineToU32(*(P_ptr->combined_affine), *tempR);
    *tempL = ApplyAffineToU32(*((P_ptr + 1)->combined_affine), *tempL);

    L[0] = LT[0];
    L[1] = LT[1];
    L[2] = LT[2];
    L[3] = LT[3];

    R[0] = RT[0];
    R[1] = RT[1];
    R[2] = RT[2];
    R[3] = RT[3];

    for (i = 0; i < swc->rounds; i++)
    {

        RT[0] = R[0];
        RT[1] = R[1];
        RT[2] = R[2];
        RT[3] = R[3];

        // start theta
        *tempR = ApplyAffineToU32(*(B_ptr->combined_affine), *tempR);

        //***************************************************************************

        //**********************************************************************************

        //***********************************************************

        //start beta
        uint32_t RC[4];
        RC[0] = lut_ptr[swc->rounds - 1 - (i)][0][RT[0]];
        RC[1] = lut_ptr[swc->rounds - 1 - (i)][1][RT[1]];
        RC[2] = lut_ptr[swc->rounds - 1 - (i)][2][RT[2]];
        RC[3] = lut_ptr[swc->rounds - 1 - (i)][3][RT[3]];

        *tempR = RC[0] ^ RC[1] ^ RC[2] ^ RC[3];

        // MatGf2 rotate = make_right_rotate_shift(32, 1, 2, 7);

        // *tempR = ApplyMatToU32(rotate, *tempR);

        //SWITCHLANE

        *tempR = ApplyMatToU32(switchmat, *tempR);

        // P for next and 

        printf("\t%08X\n", *tempR);
        CombinedAffine * tca = NULL;
        if (i==0) {
            tca = P_ptr+1;
        } else {
            tca = P_ptr+1;
        }
        RT[0] = ApplyAffineToU8((tca)->sub_affine[0], RT[0]);
        RT[1] = ApplyAffineToU8((tca)->sub_affine[1], RT[1]);
        RT[2] = ApplyAffineToU8((tca)->sub_affine[2], RT[2]);
        RT[3] = ApplyAffineToU8((tca)->sub_affine[3], RT[3]);


  
    
        swan_wb_unit tempd[4];

        *((swan_wb_semi *)tempd) = ApplyAffineToU32(*((P_ptr + 2)->combined_affine), ApplyAffineToU32(*((P_ptr)->combined_affine_inv), *((uint32_t *)R)));

        R[0] = L[0] ^ RT[0];
        R[1] = L[1] ^ RT[1];
        R[2] = L[2] ^ RT[2];
        R[3] = L[3] ^ RT[3];

        L[0] = tempd[0];
        L[1] = tempd[1];
        L[2] = tempd[2];
        L[3] = tempd[3];

        swan_wb_semi LP;
        MatGf2 mat_lp = NULL;
        if (swc->weak_or_strong) {
            RT[0] = R[0];
            RT[1] = R[1];
            RT[2] = R[2];
            RT[3] = R[3];
            LP = *tempR;
            swan_wb_unit ULP[4];
            swan_wb_semi *ulp_ptr = (swan_wb_semi *)ULP;
            *ulp_ptr = ApplyAffineToU32(*(PQ_ptr->combined_affine), ApplyMatToU32(((P_ptr + 1)->combined_affine_inv->linear_map), *tempR));
            *((swan_wb_semi *)R) = *ulp_ptr;

            MatGf2 mat_x = NULL;
            ReAllocatedMatGf2(32, 1, &mat_x);
            InitVecFromBit(LP, mat_x);
            MatGf2Add(GenMatGf2Mul((tca)->combined_affine->linear_map, (tca)->combined_affine_inv->vector_translation), mat_x, &mat_x);
            LP = get32FromVec(mat_x);

        } else {
            MatGf2 mat_x = NULL;
            ReAllocatedMatGf2(32, 1, &mat_x);
            InitVecFromBit(*((swan_wb_semi *)R), mat_x);
            MatGf2Add(GenMatGf2Mul((tca)->combined_affine->linear_map, (tca)->combined_affine_inv->vector_translation), mat_x, &mat_x);
            *((swan_wb_semi *)R) = get32FromVec(mat_x);
        }


        P_ptr++;
        B_ptr++;
        i++;


        // next round 

        if (swc->weak_or_strong) {
            *tempR = LP;
        } else {
            RT[0] = R[0];
            RT[1] = R[1];
            RT[2] = R[2];
            RT[3] = R[3];
        }

        // start theta
        *tempR = ApplyAffineToU32(*(B_ptr->combined_affine), *tempR);
        //***************************************************************************

        //**********************************************************************************

        //***********************************************************

        //start beta
        RC[0] = lut_ptr[swc->rounds - 1 - (i)][0][RT[0]];
        RC[1] = lut_ptr[swc->rounds - 1 - (i)][1][RT[1]];
        RC[2] = lut_ptr[swc->rounds - 1 - (i)][2][RT[2]];
        RC[3] = lut_ptr[swc->rounds - 1 - (i)][3][RT[3]];

        *tempR = RC[0] ^ RC[1] ^ RC[2] ^ RC[3];

        // MatGf2 rotate = make_right_rotate_shift(32, 1, 2, 7);

        // *tempR = ApplyMatToU32(rotate, *tempR);

        //SWITCHLANE

        *tempR = ApplyMatToU32(switchmat, *tempR);

        // P for next and 

        RT[0] = ApplyAffineToU8((P_ptr + 1)->sub_affine[0], RT[0]);
        RT[1] = ApplyAffineToU8((P_ptr + 1)->sub_affine[1], RT[1]);
        RT[2] = ApplyAffineToU8((P_ptr + 1)->sub_affine[2], RT[2]);
        RT[3] = ApplyAffineToU8((P_ptr + 1)->sub_affine[3], RT[3]);

        if (swc->weak_or_strong)
        {
            *((swan_wb_semi *)tempd) = ApplyAffineToU32(*((P_ptr + 2)->combined_affine), ApplyAffineToU32(*((PQ_ptr)->combined_affine_inv), *((uint32_t *)R)));
        }
        else
        {
            *((swan_wb_semi *)tempd) = ApplyAffineToU32(*((P_ptr + 2)->combined_affine), ApplyAffineToU32(*((P_ptr)->combined_affine_inv), *((uint32_t *)R)));
        }


        R[0] = L[0] ^ RT[0];
        R[1] = L[1] ^ RT[1];
        R[2] = L[2] ^ RT[2];
        R[3] = L[3] ^ RT[3];

        L[0] = tempd[0];
        L[1] = tempd[1];
        L[2] = tempd[2];
        L[3] = tempd[3];

        mat_x = NULL;
        ReAllocatedMatGf2(32, 1, &mat_x);
        InitVecFromBit(*((swan_wb_semi *)R), mat_x);
        MatGf2Add(GenMatGf2Mul((P_ptr + 1)->combined_affine->linear_map, (P_ptr + 1)->combined_affine_inv->vector_translation), mat_x, &mat_x);
        *((swan_wb_semi *)R) = get32FromVec(mat_x);

        P_ptr++;
        B_ptr++;
        PQ_ptr++;

    }

    *((swan_wb_semi *)R) = ApplyAffineToU32(*((P_ptr + 0)->combined_affine_inv), *((swan_wb_semi *)R));
    *((swan_wb_semi *)L) = ApplyAffineToU32(*((P_ptr + 1)->combined_affine_inv), *((swan_wb_semi *)L));

    out[0] = L[0];
    out[1] = L[1];
    out[2] = L[2];
    out[3] = L[3];

    out[4] = R[0];
    out[5] = R[1];
    out[6] = R[2];
    out[7] = R[3];

    return 0;
}

int swan_whitebox_release(swan_whitebox_content *swc)
{
    // TODO:
    // AffineTransformFree(swc->)

    int i, j;
    CombinedAffine *B_ptr = swc->B;
    for (i = 0; i < swc->rounds; i++)
    {
        combined_affine_free(B_ptr++);
    }
    free(swc->B);
    swc->B = NULL;
    CombinedAffine *P_ptr = swc->P;
    for (i = 0; i < swc->rounds + 2; i++)
    {
        combined_affine_free(P_ptr++);
    }
    free(swc->P);
    swc->P = NULL;


    //free memory
    free(swc->lut);
    swc->lut = NULL;

    return 0;
}

int swan_wb_export_to_bytes(const swan_whitebox_content *swc, uint8_t **dest)
{
    if (*dest != NULL)
        return -1;
    int sz = 0;
    sz = sizeof(swan_whitebox_content);
    // sz += swc->rounds * swc->aff_in_round * sizeof(AffineTransform);    //round_aff
    sz += swc->rounds * swc->piece_count * sizeof(swan_wb_semi) * 256; //LUT
    sz += 2 * swc->piece_count * sizeof(swan_wb_unit) * 256;           //SE
    sz += 2 * swc->piece_count * sizeof(swan_wb_unit) * 256;           //EE

    void **B_ptr_list = malloc(swc->rounds  * sizeof(void *));
    void **P_ptr_list = malloc((2 + swc->rounds) * sizeof(void *));
    void **P_ptr_sub_list = malloc((swc->rounds+2) * sizeof(void *) * 4);
    void **P_ptr_inv_list = malloc((swc->rounds+2) * sizeof(void *));

    int i;
    int j;
    int sum = 0;
    for (i = 0; i < swc->rounds ; i++)
    {

        B_ptr_list[i] = ExportAffineToStr((swc->B+i)->combined_affine);
        sz += *(uint32_t *)B_ptr_list[i];
    }

    for (i = 0; i < swc->rounds + 2; i++)
    {

        P_ptr_list[i] = ExportAffineToStr((swc->P+i)->combined_affine);
        sz += *(uint32_t *)P_ptr_list[i];
    }

    for (i = 0; i < swc->rounds + 2; i++)
    {

        P_ptr_inv_list[i] = ExportAffineToStr((swc->P + i)->combined_affine_inv);
        sz += *(uint32_t *)P_ptr_inv_list[i];
    }

    for (i = 0; i < (swc->rounds + 2); i++)
    {
        for(j= 0;j<4;j++,sum++){
            P_ptr_sub_list[sum] = ExportAffineToStr((swc->P + i)->sub_affine + j);
            sz += *(uint32_t *)P_ptr_sub_list[sum];
        }
       
    }
    



    *dest = malloc(sz);
    *((uint32_t *)*dest) = sz;
    uint8_t *ds = *dest + sizeof(uint32_t);
    memcpy(ds, swc, sizeof(swan_whitebox_content));
    ds += sizeof(swan_whitebox_content);
    int k;
    k = swc->rounds * swc->piece_count * sizeof(swan_wb_semi) * 256;
    memcpy(ds, swc->lut, k);
    ds += k;

    k = 2 * swc->piece_count * sizeof(swan_wb_unit) * 256;
    memcpy(ds, swc->SE, k);
    ds += k;

    k = 2 * swc->piece_count *  sizeof(swan_wb_unit) * 256;
    memcpy(ds, swc->EE, k);
    ds += k;

    for (i = 0; i < swc->rounds; i++)
    {
        k = *(uint32_t *)B_ptr_list[i];
        memcpy(ds, B_ptr_list[i], k);
        ds += k;
    }

    for (i = 0; i < swc->rounds + 2; i++)
    {
        k = *(uint32_t *)P_ptr_list[i];
        memcpy(ds, P_ptr_list[i], k);
        ds += k;
    }

    for (i = 0; i < swc->rounds + 2; i++)
    {
        k = *(uint32_t *)P_ptr_inv_list[i];
        memcpy(ds, P_ptr_inv_list[i], k);
        ds += k;
    }
    sum = 0;
    for (i = 0; i < swc->rounds + 2;i++){
        for (j = 0; j < 4; j++, sum++)
        {
            k = *(uint32_t *)P_ptr_sub_list[sum];
            memcpy(ds, P_ptr_sub_list[sum], k);
            ds += k;
        }
    }

        return sz;
}

int swan_wb_import_from_bytes(const uint8_t *source, swan_whitebox_content *swc)
{
    const void *ptr = source;
    ptr += sizeof(uint32_t);
    
    memcpy(swc, ptr, sizeof(swan_whitebox_content));
    ptr += sizeof(swan_whitebox_content);

    int i;
    int j;
    int k;
    int sum = 0;
    k = swc->rounds * swc->piece_count * sizeof(swan_wb_semi) * 256;
    swc->lut = (swan_wb_semi(*)[4][256])malloc(k);
    memcpy(swc->lut, ptr, k);
    ptr += k;

    k = 2 * swc->piece_count * sizeof(swan_wb_unit) * 256;
    memcpy(swc->SE, ptr, k);
    ptr += k;

    k = 2 * swc->piece_count * sizeof(swan_wb_unit) * 256;
    memcpy(swc->EE, ptr, k);
    ptr += k;

    swc->P = (CombinedAffine *)malloc((swc->rounds + 2) * sizeof(CombinedAffine));

    swc->B = (CombinedAffine *)malloc(swc->rounds * sizeof(CombinedAffine));

    CombinedAffine *B_ptr = swc->B;
    CombinedAffine *P_ptr = swc->P;

    for (i = 0; i < (swc->rounds); i++)
    {

        combined_affine_init(B_ptr++, SWAN_PIECE_BIT, swc->piece_count);
       
    }
    for(i = 0;i < (swc->rounds + 2); i++){
        combined_affine_init(P_ptr++, SWAN_PIECE_BIT, swc->piece_count);
    }
 



    for (i = 0; i < swc->rounds ; i++)
    {
        uint32_t aff_sz = *((uint32_t *)ptr);
        *((swc->B+i)->combined_affine) = ImportAffineFromStr(ptr);
        ptr += aff_sz;
    }

    for (i = 0; i < swc->rounds + 2; i++)
    {
        uint32_t aff_sz = *((uint32_t *)ptr);
        *((swc->P+i)->combined_affine) = ImportAffineFromStr(ptr);
        ptr += aff_sz;
    }
    for (i = 0; i < swc->rounds + 2; i++)
    {
        uint32_t aff_sz = *((uint32_t *)ptr);
        *((swc->P + i)->combined_affine_inv) = ImportAffineFromStr(ptr);
        ptr += aff_sz;
    }

    for (i = 0; i < swc->rounds + 2; i++)
    {
        for (j = 0; j < 4; j++, sum++)
        {
            uint32_t aff_sz = *((uint32_t *)ptr);
            *((swc->P+i)->sub_affine +j) = ImportAffineFromStr(ptr);
            ptr += aff_sz;
        }
    }

    return 0;
}
