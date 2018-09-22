typedef float_t mpli_dt;

typedef struct _mpli_point_t                           
{                                                      
	mpli_dt x, y;
}                                                      
mpli_point_t;                                          
													   
													   
typedef struct _mpli_t                                 
{                                                      
	const mpli_point_t *  	table;                          
	uint8_t         		count;                             
}                                                      
mpli_t;                                                
													   
mpli_dt mpli_calc( const mpli_t *p_table, mpli_dt in  );
