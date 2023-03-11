(set-logic BV)

(define-fun ehad ((x (_ BitVec 64))) (_ BitVec 64)
    (bvlshr x #x0000000000000001))
(define-fun arba ((x (_ BitVec 64))) (_ BitVec 64)
    (bvlshr x #x0000000000000004))
(define-fun shesh ((x (_ BitVec 64))) (_ BitVec 64)
    (bvlshr x #x0000000000000010))
(define-fun smol ((x (_ BitVec 64))) (_ BitVec 64)
    (bvshl x #x0000000000000001))
(define-fun im ((x (_ BitVec 64)) (y (_ BitVec 64)) (z (_ BitVec 64))) (_ BitVec 64)
    (ite (= x #x0000000000000001) y z))
(synth-fun f ((x (_ BitVec 64))) (_ BitVec 64)
    ((Start (_ BitVec 64)))
    ((Start (_ BitVec 64) (#x0000000000000000 #x0000000000000001 x (bvnot Start) (smol Start) (ehad Start) (arba Start) (shesh Start) (bvand Start Start) (bvor Start Start) (bvxor Start Start) (bvadd Start Start) (im Start Start Start)))))

(constraint (= (f #xFB01B148BF1B7E60) #xFFFFF04FE4EB740E))
(constraint (= (f #x9138F23320FA19DC) #xFFFFF6EC70DCCDF0))
(constraint (= (f #x156AB8DF6C92C6A4) #xFFFFFEA954720936))
(constraint (= (f #x6BB6254007A89804) #xFFFFF9449DABFF85))
(constraint (= (f #x592902BF54E1131C) #xFFFFFA6D6FD40AB1))
(constraint (= (f #xE79CFD9707119C7F) #xFFFFF18630268F8E))
(constraint (= (f #xCA4879109D280B73) #xFFFFF35B786EF62D))
(constraint (= (f #x3044AF69F39DBDF7) #xFFFFFCFBB50960C6))
(constraint (= (f #xAA5162C5A6A61C87) #xFFFFF55AE9D3A595))
(constraint (= (f #x2CC3E9A65B82534F) #xFFFFFD33C1659A47))
(constraint (= (f #x1C20A934412E90DA) #x38415268825D21B4))
(constraint (= (f #x4D1FA18CA313BF32) #x9A3F431946277E64))
(constraint (= (f #x5A4318A9FF6952A2) #xB4863153FED2A544))
(constraint (= (f #x285347782F8242C6) #x50A68EF05F04858C))
(constraint (= (f #x95BDA4716536096E) #x2B7B48E2CA6C12DC))
(constraint (= (f #x0000000000000000) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000038) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000030) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000024) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000020) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x83C4911B1E6A2E3D) #x078922363CD45C7A))
(constraint (= (f #xC8F3A9B5D357AAB5) #x91E7536BA6AF556A))
(constraint (= (f #x30C8C0FA43E76A59) #x619181F487CED4B2))
(constraint (= (f #x9C5F68B1DC9F69D1) #x38BED163B93ED3A2))
(constraint (= (f #xE8C5A72D8427C66D) #xD18B4E5B084F8CDA))
(constraint (= (f #xAAAAAAAAAAAAAAAB) #xFFFFFAAAAAAAAAAA))
(constraint (= (f #x000000000000002B) #x0000000000000056))
(constraint (= (f #x000000000000003B) #x0000000000000076))
(constraint (= (f #x0000000000000027) #x000000000000004E))
(constraint (= (f #x0000000000000037) #x000000000000006E))
(constraint (= (f #x0000000000000026) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x000000000000003A) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000032) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000022) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x000000000000003E) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000039) #x0000000000000072))
(constraint (= (f #x000000000000003D) #x000000000000007A))
(constraint (= (f #x0000000000000021) #x0000000000000042))
(constraint (= (f #x0000000000000029) #x0000000000000052))
(constraint (= (f #x73FC8A1BFB87F3E5) #xE7F91437F70FE7CA))
(constraint (= (f #x64C5B99FCF928744) #xFFFFF9B3A4660306))
(constraint (= (f #x858BDF5B01F62A4F) #xFFFFF7A7420A4FE0))
(constraint (= (f #x2C1088DEDBE0E1BE) #x582111BDB7C1C37C))
(constraint (= (f #x160ED5CB832D8451) #x2C1DAB97065B08A2))
(constraint (= (f #x5FFE62C8730FA12A) #xBFFCC590E61F4254))
(constraint (= (f #xD66834AF503D0952) #xACD0695EA07A12A4))
(constraint (= (f #xE3BCBCCD530D78A5) #xC779799AA61AF14A))
(constraint (= (f #x686AC418F5B4947C) #xFFFFF97953BE70A4))
(constraint (= (f #x309DA1CF02E3CF13) #xFFFFFCF625E30FD1))
(constraint (= (f #xAAAAAAAAAAAAAAAB) #xFFFFFAAAAAAAAAAA))
(constraint (= (f #x000000000000002D) #x000000000000005A))
(constraint (= (f #x0000000000000030) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x0000000000000036) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #x000000000000003F) #x000000000000007E))
(constraint (= (f #xE12F42FA5581EA3C) #xFFFFF1ED0BD05AA7))
(constraint (= (f #x82106403206B4083) #xFFFFF7DEF9BFCDF9))
(constraint (= (f #x8CE5EF5ECD1B568E) #x19CBDEBD9A36AD1C))
(constraint (= (f #x88CBB454962EC166) #x119768A92C5D82CC))
(constraint (= (f #x0000000000000013) #xFFFFFFFFFFFFFFFF))
(constraint (= (f #xCF307E658B822415) #x9E60FCCB1704482A))
(constraint (= (f #x8A4A529555529253) #xFFFFF75B5AD6AAAA))
(constraint (= (f #xA02AA2144811014B) #xFFFFF5FD55DEBB7E))
(constraint (= (f #x950544A8228A5203) #xFFFFF6AFABB57DD7))
(constraint (= (f #x0000000000000022) #xFFFFFFFFFFFFFFFF))

(check-synth)

